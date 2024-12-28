//
// Copyright (c) 2023, 2024 Humanitarian OpenStreetMap Team
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
#include "underpassconfig.hh"
#include <mutex>

using namespace queryraw;

namespace bootstrap {

/// \struct BootstrapTask
/// \brief Represents a bootstrap task
struct BootstrapTask {
    std::vector<std::string> query;
    int processed = 0;
};

struct RelationTask {
    int taskIndex;
    std::shared_ptr<std::vector<BootstrapTask>> tasks;
    std::shared_ptr<std::vector<OsmRelation>> relations;
};

class Bootstrap {
  public:
    Bootstrap(void);
    ~Bootstrap(void){};

    static const underpassconfig::UnderpassConfig &config;
    
    void start(const underpassconfig::UnderpassConfig &config);
    void processRelations();
    void processNodes();

    // This thread get started for every page of relations
    void threadBootstrapRelationTask(RelationTask relationTask);
    std::shared_ptr<std::vector<std::string>> allTasksQueries(std::shared_ptr<std::vector<BootstrapTask>> tasks);
    
    std::shared_ptr<QueryRaw> queryraw;
    std::shared_ptr<Pq> db;
    bool norefs;
    unsigned int concurrency;
    unsigned int page_size;
    std::string pbf;
};

static std::mutex tasks_change_mutex;

}
