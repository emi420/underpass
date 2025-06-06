//
// Copyright (c) 2023, 2024 Humanitarian OpenStreetMap Team
// Copyright (c) 2025 Emilio Mariscal
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

#include "underpassconfig.hh"
#include "raw/queryraw.hh"
#include "bootstrap/osmprocessor.hh"
#include "bootstrap/bootstrap.hh"
#include "data/pq.hh"
#include "utils/log.hh"

using namespace queryraw;
using namespace underpassconfig;
using namespace logger;
using namespace osmprocessor;

namespace bootstrap {

    Bootstrap::Bootstrap(void) {}

    bool
    Bootstrap::connect(const std::string &db_url) {
        db = std::make_shared<Pq>();
        if (!db->connect(db_url)) {
            log_error("Could not connect to Underpass DB, aborting bootstrapping thread!");
            return false;
        }
        queryraw = std::make_shared<QueryRaw>(db);
        return true;
    }

    void
    Bootstrap::processPBF(std::string &pbf, int page_size, int concurrency) {
        std::cout << "Processing PBF ... (" << pbf << ")" << std::endl;

        auto rawTasker = std::make_shared<RawTasker>(db, queryraw, page_size, concurrency);
        auto osmProcessor = OsmProcessor(rawTasker, pbf);

        std::cout << "Processing nodes and ways ..." << std::endl;
        osmProcessor.nodesAndWays();

        std::cout << "Processing relations  ..." << std::endl;
        osmProcessor.relations();

        std::cout << "Processing relations  (geometries)..." << std::endl;
        osmProcessor.relationsGeometries();

    }

    void
    Bootstrap::start(const underpassconfig::UnderpassConfig &config) {
        if (!connect(config.underpass_db_url)) {
            std::cout << "Error trying to connect to the database" << std::endl;
            exit(0);
        }
        queryraw = std::make_shared<QueryRaw>(db);
        std::string pbf = config.import;
        if (!config.latest && !pbf.empty()) {
            processPBF(pbf, config.bootstrap_page_size, config.concurrency);
        }
    }

    boost::posix_time::ptime
    Bootstrap::getLatestTimestamp(void) {
        return queryraw->getLatestTimestamp();
    }

    void
    Bootstrap::initializeDB(void) {
        std::string filepath = ETCDIR;
        filepath += "/setup/underpass.sql";
        db->queryFile(filepath);
    }

    void
    Bootstrap::createDBIndexes(void) {
        std::string filepath = ETCDIR;
        filepath += "/setup/indexes.sql";
        db->queryFile(filepath);
    }

}


