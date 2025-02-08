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

    // std::shared_ptr<std::vector<std::string>>
// Bootstrap::allTasksQueries(std::shared_ptr<std::vector<BootstrapTask>> tasks) {
//     auto queries = std::make_shared<std::vector<std::string>>();
//     for (auto it = tasks->begin(); it != tasks->end(); ++it) {
//         for (auto itt = it->query.begin(); itt != it->query.end(); ++itt) {
//             queries->push_back(*itt);
//         }
//     }
//     return queries; 
// }


    RawTasker::RawTasker(std::shared_ptr<Pq> db, std::shared_ptr<QueryRaw> queryraw) {
        this->db = db;
        this->queryraw = queryraw;
    }

    void
    RawTasker::apply(OsmNode &osmNode) {
        std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(osmNode);
        for (const auto& q : *query) {
            queries.push_back(q);
        }

        std::string queries_str = "";
        for (auto it = queries.begin(); it != queries.end(); ++it) {
            queries_str += *it;
        }
        queries.clear();
        db->query(queries_str);
    }

    void
    RawTasker::apply(OsmWay &osmWay) {
        std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(osmWay);
        for (const auto& q : *query) {
            queries.push_back(q);
        }

        std::string queries_str = "";
        for (auto it = queries.begin(); it != queries.end(); ++it) {
            queries_str += *it;
        }
        queries.clear();
        db->query(queries_str);
    }

    void
    RawTasker::apply(OsmRelation &osmRelation) {
        std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(osmRelation);
        for (const auto& q : *query) {
            queries.push_back(q);
        }

        std::string queries_str = "";
        for (auto it = queries.begin(); it != queries.end(); ++it) {
            queries_str += *it;
        }
        queries.clear();
        db->query(queries_str);
    }

}