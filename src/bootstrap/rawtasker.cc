//
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

#include "raw/queryraw.hh"
#include "bootstrap/rawtasker.hh"
#include "data/pq.hh"
// #include <boost/asio.hpp>
// #include <boost/thread/thread.hpp>
// #include <boost/asio/thread_pool.hpp>

namespace rawtasker {

    RawTasker::RawTasker(std::shared_ptr<Pq> db, std::shared_ptr<QueryRaw> queryraw) {
        this->db = db;
        this->queryraw = queryraw;
    }

    void
    RawTasker::finish() {
        checkNodes(true);
        checkWays(true);
        checkRelations(true);
    }

    void
    RawTasker::checkNodes() {
        checkNodes(false);
    }
    void
    RawTasker::checkNodes(bool finish) {
        if (finish || nodecache.size() > 1000) {
            std::string queries;
            for (const OsmNode& node : nodecache) {
                std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(node);
                for (const auto& q : *query) {
                    queries.append(q);
                }
            }
            db->query(queries);
            nodecache.clear();
        }
    }

    void
    RawTasker::checkWays() {
        checkWays(false);
    }
    void
    RawTasker::checkWays(bool finish) {
        if (finish || waycache.size() > 1000) {
            std::string queries;
            for (const OsmWay& way : waycache) {
                std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(way);
                for (const auto& q : *query) {
                    queries.append(q);
                }
            }
            db->query(queries);
            waycache.clear();
        }
    }

    void
    RawTasker::checkRelations() {
        checkRelations(false);
    }
    void
    RawTasker::checkRelations(bool finish) {
        if (finish || relcache.size() > 1000) {
            std::string queries;
            for (const OsmRelation& rel : relcache) {
                std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(rel);
                for (const auto& q : *query) {
                    queries.append(q);
                }
            }
            db->query(queries);
            relcache.clear();
        }
    }

    void
    RawTasker::apply(OsmNode &osmNode) {
        nodecache.push_back(osmNode);
        checkNodes();
    }

    void
    RawTasker::apply(OsmWay &osmWay) {
        waycache.push_back(osmWay);
        checkWays();
    }

    void
    RawTasker::apply(OsmRelation &osmRelation) {
        relcache.push_back(osmRelation);
        checkRelations();
    }

}