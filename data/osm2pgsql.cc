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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <chrono>
#include <string>

#include <boost/iostreams/copy.hpp>
#include <boost/process.hpp>
using namespace boost::process;

#include "osm2pgsql.hh"

#include "log.hh"
using namespace logger;

namespace osm2pgsql {

const std::string Osm2Pgsql::OSM2PGSQL_DEFAULT_SCHEMA_NAME = "osm2pgsql_pgsql";

logger::LogFile &dbglogfile = logger::LogFile::getDefaultInstance();

Osm2Pgsql::Osm2Pgsql(const std::string &_dburl, const std::string &schema)
    : schema(schema)
{
    if (!connect(_dburl)) {
        log_error(_("Could not connect to osm2pgsql server %1%"), _dburl);
    }
}

ptime
Osm2Pgsql::getLastUpdate()
{
    if (last_update.is_not_a_date_time()) {
        getLastUpdateFromDb();
    }
    return last_update;
}

bool
Osm2Pgsql::updateDatabase(const std::string &osm_changes)
{
    if (!sdb->is_open()) {
        log_error(
            _("Update error: connection to osm2pgsql server '%1%' is closed"),
            dburl);
        return false;
    }
    // -l for 4326
    std::string osm2pgsql_update_command{
        "osm2pgsql -l --append -r xml -s -C 300 -G --hstore --middle-schema=" +
        schema + " --output-pgsql-schema=" + schema + " -d postgresql://"};
    // Append DB url in the form: postgresql://[user[:password]@][netloc][:port][,...][/dbname][?param1=value1&...]
    osm2pgsql_update_command.append(dburl);
    // Read from stdin
    osm2pgsql_update_command.append(" -");

    log_debug(_("Executing osm2pgsql, command: %1%"), osm2pgsql_update_command);

    ipstream out;
    ipstream err;
    opstream in;

    child osm2pgsql_update_process(osm2pgsql_update_command, std_out > out,
                                   std_err > err, std_in < in);

    in.write(osm_changes.data(), osm_changes.size());
    in.close();
    in.pipe().close();

    // FIXME: make wait_for duration an arg
    bool result{osm2pgsql_update_process.running() &&
                osm2pgsql_update_process.wait_for(std::chrono::minutes{1}) &&
                osm2pgsql_update_process.exit_code() == EXIT_SUCCESS};
    if (!result) {
        std::stringstream err_mesg;
        boost::iostreams::copy(err, err_mesg);
        log_error(_("Error running osm2pgsql command %1%\n%2%"),
                  osm2pgsql_update_command, err_mesg.str());
    }
    return result;
}

bool
Osm2Pgsql::connect(const std::string &_dburl)
{
    const bool result{pq::Pq::connect(_dburl)};
    if (result) {
        dburl = _dburl;
    } else {
        dburl.clear();
    }
    return result;
}

bool
Osm2Pgsql::getLastUpdateFromDb()
{
    if (sdb->is_open()) {
        const std::string sql{R"sql(
        SELECT MAX(foo.ts) AS ts FROM(
          SELECT MAX(tags -> 'osm_timestamp') AS ts FROM osm2pgsql_pgsql.planet_osm_point
          UNION
          SELECT MAX(tags -> 'osm_timestamp') AS ts FROM osm2pgsql_pgsql.planet_osm_line
          UNION
          SELECT MAX(tags -> 'osm_timestamp') AS ts FROM osm2pgsql_pgsql.planet_osm_polygon
          UNION
          SELECT MAX(tags -> 'osm_timestamp') AS ts FROM osm2pgsql_pgsql.planet_osm_roads
        ) AS foo
      )sql"};
        try {
            const auto result{query(sql)};
            const auto row{result.at(0)};
            if (row.size() != 1) {
                return false;
            }
            auto timestamp{row[0].as<std::string>()};
            timestamp[10] = ' '; // Drop the 'T' in the middle
            timestamp.erase(19); // Drop the final 'Z'
            last_update = time_from_string(timestamp);
            return last_update != not_a_date_time;
        } catch (std::exception const &e) {
            log_error(_("Error getting last update from osm2pgsql DB: %1%"),
                      e.what());
        }
        return false;
    } else {
        return false;
    }
}

} // namespace osm2pgsql
