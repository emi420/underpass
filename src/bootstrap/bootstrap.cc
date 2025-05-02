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

    void
    Bootstrap::connect(const std::string &db_url) {
        std::cout << "Connecting to underpass database ... " << std::endl;
        db = std::make_shared<Pq>();
        if (!db->connect(db_url)) {
            log_error("Could not connect to Underpass DB, aborting bootstrapping thread!");
            return;
        }
        queryraw = std::make_shared<QueryRaw>(db);
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

        // TODO: launch replicator (optional)

    }

    void
    Bootstrap::start(const underpassconfig::UnderpassConfig &config) {
        connect(config.underpass_db_url);
        queryraw = std::make_shared<QueryRaw>(db);
        queryraw->onConflict = false;
        std::string pbf = config.import;
        if (!pbf.empty()) {
            processPBF(pbf, config.bootstrap_page_size, config.concurrency);
        }
    }

    boost::posix_time::ptime
    Bootstrap::getLatestTimestamp(void) {
        return queryraw->getLatestTimestamp();
    }

}


