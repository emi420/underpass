//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
//
// This file is part of Underpass.
//
//     Underpass is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Underpass is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.
//

#include <dejagnu.h>
#include <iostream>
#include <pqxx/pqxx>
#include <string>

#include "data/osm2pgsql.hh"
#include "hottm.hh"
#include "log.hh"

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

using namespace osm2pgsql;
using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace boost::filesystem;

TestState runtest;

class TestOsm2Pgsql : public Osm2Pgsql {
  public:
    TestOsm2Pgsql() = default;

    //! Clear the test DB and fill it with with initial test data
    bool init_test_case()
    {

        logger::LogFile &dbglogfile = logger::LogFile::getDefaultInstance();
        dbglogfile.setVerbosity();

        const std::string dbconn{getenv("UNDERPASS_TEST_DB_CONN")
                                     ? getenv("UNDERPASS_TEST_DB_CONN")
                                     : ""};
        source_tree_root = getenv("UNDERPASS_SOURCE_TREE_ROOT")
                               ? getenv("UNDERPASS_SOURCE_TREE_ROOT")
                               : SRCDIR;

        const std::string test_osm2pgsql_db_name{"osm2pgsql_test"};

        {
            pqxx::connection conn{dbconn};
            pqxx::nontransaction worker{conn};
            worker.exec0("DROP DATABASE IF EXISTS " + test_osm2pgsql_db_name);
            worker.exec0("CREATE DATABASE " + test_osm2pgsql_db_name);
            worker.commit();
        }

        {
            pqxx::connection conn{dbconn + " dbname=" + test_osm2pgsql_db_name};
            pqxx::nontransaction worker{conn};
            worker.exec0("CREATE EXTENSION postgis");
            worker.exec0("CREATE EXTENSION hstore");

            // Create schema
            const path base_path{source_tree_root / "testsuite"};
            const auto schema_path{base_path / "testdata" /
                                   "pgsql_test_schema.sql"};
            std::ifstream schema_definition(schema_path);
            std::string sql((std::istreambuf_iterator<char>(schema_definition)),
                            std::istreambuf_iterator<char>());

            assert(!sql.empty());
            worker.exec0(sql);

            // Load a minimal data set for testing
            const auto data_path{base_path / "testdata" /
                                 "pgsql_test_data.sql.gz"};
            std::ifstream data_definition(data_path, std::ios_base::in |
                                                         std::ios_base::binary);
            boost::iostreams::filtering_streambuf<boost::iostreams::input>
                inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            inbuf.push(data_definition);
            std::istream instream(&inbuf);
            std::string data_sql((std::istreambuf_iterator<char>(instream)),
                                 std::istreambuf_iterator<char>());
            assert(!data_sql.empty());
            worker.exec0(data_sql);

            // Load changes
            const auto changes_data_path{base_path / "testdata" /
                                         "simple_test.osc"};
            std::ifstream osm_change(changes_data_path,
                                     std::ios_base::in | std::ios_base::binary);
            boost::iostreams::filtering_istream osm_change_inbuf;
            osm_change_inbuf.push(osm_change);
            std::stringstream sstream;
            auto bytes_written =
                boost::iostreams::copy(osm_change_inbuf, sstream);
            osm_changes = sstream.str();
            assert(bytes_written > 0);
        }

        return true;
    };

    std::string source_tree_root;
    std::string osm_changes;
};

int
main(int argc, char *argv[])
{

    // Test preconditions

    logger::LogFile &dbglogfile = logger::LogFile::getDefaultInstance();
    dbglogfile.setLogFilename("");
    dbglogfile.setVerbosity();

    TestOsm2Pgsql testosm2pgsql;

    // Test that default constructed object have a schema set
    assert(testosm2pgsql.getSchema().compare(
               testosm2pgsql.OSM2PGSQL_DEFAULT_SCHEMA_NAME) == 0);

    testosm2pgsql.init_test_case();

    const std::string test_osm2pgsql_db_name{"osm2pgsql_test"};
    std::string osm2pgsql_conn;
    if (getenv("PGHOST") && getenv("PGUSER") && getenv("PGPASSWORD")) {
        osm2pgsql_conn = std::string(getenv("PGUSER")) + ":" +
                         std::string(getenv("PGPASSWORD")) + "@" +
                         std::string(getenv("PGHOST")) + "/" +
                         test_osm2pgsql_db_name;
    } else {
        osm2pgsql_conn = test_osm2pgsql_db_name;
    }

    if (testosm2pgsql.connect(osm2pgsql_conn)) {
        runtest.pass("Osm2Pgsql::connect()");
    } else {
        runtest.fail("Osm2Pgsql::connect()");
        exit(EXIT_FAILURE);
    }

    const auto last_update{testosm2pgsql.getLastUpdate()};
    std::cout << "ts: " << to_iso_extended_string(last_update) << std::endl;
    if (!last_update.is_not_a_date_time() &&
        to_iso_extended_string(last_update).compare("2021-08-05T23:38:28") ==
            0) {
        runtest.pass("Osm2Pgsql::getLastUpdate()");
    } else {
        runtest.fail("Osm2Pgsql::getLastUpdate()");
        exit(EXIT_FAILURE);
    }

    // Read the osm change from file and process it
    if (!testosm2pgsql.updateDatabase(testosm2pgsql.osm_changes)) {
        runtest.fail("Osm2Pgsql::updateDatabase()");
        exit(EXIT_FAILURE);
    } else {
        // Check for changes!
        const auto results{
            testosm2pgsql.query("SELECT name, ST_X(way) AS x FROM " +
                                testosm2pgsql.OSM2PGSQL_DEFAULT_SCHEMA_NAME +
                                ".planet_osm_point ORDER BY name")};
        if (results.at(0)["name"]
                .as(std::string())
                .compare("Some interesting point new name") != 0) {
            runtest.fail("Osm2Pgsql::updateDatabase() - retrieve 0");
            exit(EXIT_FAILURE);
        }
        if (results.at(1)["name"]
                .as(std::string())
                .compare("Some other interesting point (44)") != 0) {
            runtest.fail("Osm2Pgsql::updateDatabase() - retrieve 1");
            exit(EXIT_FAILURE);
        }
        if (results.at(0)["x"].as(double()) != 3.12) {
            runtest.fail("Osm2Pgsql::updateDatabase() - retrieve X 0");
            exit(EXIT_FAILURE);
        }
        if (results.at(1)["x"].as(double()) != 3.0) {
            runtest.fail("Osm2Pgsql::updateDatabase() - retrieve X 1");
            exit(EXIT_FAILURE);
        }
        runtest.pass("Osm2Pgsql::updateDatabase()");
    }
}