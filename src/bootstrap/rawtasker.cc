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
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/thread_pool.hpp>

namespace rawtasker {

    RawTasker::RawTasker(std::shared_ptr<Pq> db, std::shared_ptr<QueryRaw> queryraw, int page_size, int concurrency) {
        this->db = db;
        this->queryraw = queryraw;
        this->page_size = page_size;
        this->concurrency = concurrency;
        this->chunk_size = page_size * concurrency;
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
        if (finish || nodecache.size() == chunk_size) {

            if (finish) {
                page_size = nodecache.size();
            }

            boost::asio::thread_pool pool(concurrency);
            std::vector<std::vector<OsmNode*>> nodesToProcess(concurrency);
            int index = 0;
            int count;

            for (int i = 0; i < concurrency; i++) {
                for (int j = 0; j < page_size; j++) {
                    if (index < nodecache.size()) {
                        nodesToProcess[i].push_back(&nodecache[index]);
                        index++;
                    } else {
                        continue;
                    }
                }
                if (nodesToProcess[i].size() > 0) {
                    boost::asio::post(pool, boost::bind(&RawTasker::threadNodeProcess, this, nodesToProcess[i]));
                }
            }
            pool.join();
            nodecache.clear();
        }
    }

    void
    RawTasker::threadNodeProcess(std::vector<OsmNode*> nodes) {
        std::string queries;
        for (const OsmNode* node : nodes) {
            std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(*node);
            stat_nodes++;
            for (const auto& q : *query) {
                queries.append(q);
            }
        }
        db->query(queries);
    }

    void
    RawTasker::checkWays() {
        checkWays(false);
    }
    void
    RawTasker::checkWays(bool finish) {
        if (finish || waycache.size() == chunk_size) {

            if (finish) {
                page_size = waycache.size();
            }

            boost::asio::thread_pool pool(concurrency);
            std::vector<std::vector<OsmWay*>> waysToProcess(concurrency);
            int index = 0;
            int count;

            for (int i = 0; i < concurrency; i++) {
                for (int j = 0; j < page_size; j++) {
                    if (index < waycache.size()) {
                        waysToProcess[i].push_back(&waycache[index]);
                        index++;
                    } else {
                        continue;
                    }
                }
                if (waysToProcess.size() > 0) {
                    boost::asio::post(pool, boost::bind(&RawTasker::threadWayProcess, this, waysToProcess[i]));
                }
            }
            pool.join();
            waycache.clear();
        }
    }

    void
    RawTasker::threadWayProcess(std::vector<OsmWay*> ways) {
        std::string queries;
        for (const OsmWay* way : ways) {
            std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(*way);
            stat_ways++;
            for (const auto& q : *query) {
                queries.append(q);
            }
        }
        db->query(queries);
    }

    void
    RawTasker::checkRelations() {
        checkRelations(false);
    }
    void
    RawTasker::checkRelations(bool finish) {
        if (finish || relcache.size() == chunk_size) {

            if (finish) {
                page_size = relcache.size();
            }

            boost::asio::thread_pool pool(concurrency);
            std::vector<std::vector<OsmRelation*>> relsToProcess(concurrency);
            int index = 0;
            int count;
            for (int i = 0; i < concurrency; i++) {
                for (int j = 0; j < page_size; j++) {
                    if (index < relcache.size()) {
                        relsToProcess[i].push_back(&relcache[index]);
                        index++;
                    } else {
                        continue;
                    }
                }
                if (relsToProcess.size() > 0) {
                    boost::asio::post(pool, boost::bind(&RawTasker::threadRelationProcess, this, relsToProcess[i]));
                }
            }
            pool.join();
            relcache.clear();
        }
    }

    void
    RawTasker::threadRelationProcess(std::vector<OsmRelation*> rels) {
        std::string queries;
        for (const OsmRelation* rel : rels) {
            std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(*rel);
            stat_rels++;
            for (const auto& q : *query) {
                queries.append(q);
            }
        }
        db->query(queries);
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