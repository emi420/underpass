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
#include "data/pq.hh"
#include "bootstrap/bootstrap.hh"
#include "underpassconfig.hh"

#include <boost/filesystem.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/function.hpp>
#include <boost/dll/import.hpp>
#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/thread_pool.hpp>
#include <mutex>
#include <boost/thread/pthread/shared_mutex.hpp>
#include <string.h>

#include <osmium/io/any_input.hpp>
#include <osmium/util/file.hpp>
#include <osmium/util/progress_bar.hpp>

#include "utils/log.hh"

using namespace queryraw;
using namespace underpassconfig;
using namespace logger;
namespace bg = boost::geometry;

typedef boost::geometry::model::d2::point_xy<double> point_t;

namespace bootstrap {

Bootstrap::Bootstrap(void) {}

std::shared_ptr<std::vector<std::string>>
Bootstrap::allTasksQueries(std::shared_ptr<std::vector<BootstrapTask>> tasks) {
    auto queries = std::make_shared<std::vector<std::string>>();
    for (auto it = tasks->begin(); it != tasks->end(); ++it) {
        for (auto itt = it->query.begin(); itt != it->query.end(); ++itt) {
            queries->push_back(*itt);
        }
    }
    return queries; 
}

// Handler to process nodes
class NodeHandler : public osmium::handler::Handler {
public:

    NodeHandler() {}

    std::vector<std::string> queries;
    std::shared_ptr<Pq> db;
    osmium::ProgressBar* progress = nullptr;
    osmium::io::Reader* reader = nullptr;

    void node(const osmium::Node& node) {
        std::string format = "INSERT INTO nodes (osm_id, geom) VALUES (%d, ST_GeomFromText('%s', 4326)) ON CONFLICT DO NOTHING; ";
        boost::format fmt(format);

        fmt % node.id();

        OsmNode osmNode(node.location().lat(), node.location().lon());
        std::stringstream ss;
        ss << std::setprecision(12) << bg::wkt(osmNode.point);
        std::string geometry = ss.str();
        fmt % geometry;

        queries.push_back(fmt.str());
        if (queries.size() == 500) {
            std::string queries_str = "";
            for (auto it = queries.begin(); it != queries.end(); ++it) {
                queries_str += *it;
            }
            queries.clear();
            db->query(queries_str);
        }

        if (progress && reader) {
            progress->update(reader->offset());
        }
    }
};


void
Bootstrap::start(const underpassconfig::UnderpassConfig &config) {
    std::cout << "Connecting to underpass database ... " << std::endl;
    db = std::make_shared<Pq>();
    if (!db->connect(config.underpass_db_url)) {
        log_error("Could not connect to Underpass DB, aborting bootstrapping thread!");
        return;
    }

    queryraw = std::make_shared<QueryRaw>(db);
    page_size = config.bootstrap_page_size;
    concurrency = config.concurrency;
    norefs = config.norefs;
    pbf = config.pbf;

    if (!pbf.empty()) {
        processNodes();
        processRelations();
        return;
    }
    std::cout << "Usage: underpass --bootstrap --pbf <PBF file>" << std::endl;
    return;
}

void
Bootstrap::processNodes() {
    std::cout << "Reading PBF: " << pbf << std::endl;

    const osmium::io::File input_file{pbf};
    osmium::io::Reader reader{input_file};

    std:: cout << "Processing Nodes ..." << std::endl;

    osmium::ProgressBar progress{reader.file_size(), osmium::isatty(2)};
    NodeHandler handler;
    handler.db = db;
    handler.progress = &progress;
    handler.reader = &reader;
    osmium::apply(reader, handler);

    progress.done();
    reader.close();

}

void
Bootstrap::processRelations() {

    std::cout << "Processing relations ... ";
    long int total = queryraw->getCount("relations");
    long int count = 0;
    int num_chunks = total / page_size;

    long lastid = 0;

    int concurrentTasks = concurrency;
    int percentage = 0;

    for (int chunkIndex = 0; chunkIndex <= (num_chunks/concurrentTasks); chunkIndex++) {

        percentage = (count * 100) / total;

        auto relations = std::make_shared<std::vector<OsmRelation>>();
        relations = queryraw->getRelationsFromDB(lastid, concurrency * page_size);
        auto tasks = std::make_shared<std::vector<BootstrapTask>>(concurrentTasks);
        boost::asio::thread_pool pool(concurrentTasks);
        for (int taskIndex = 0; taskIndex < concurrentTasks; taskIndex++) {
            auto taskRelations = std::make_shared<std::vector<OsmRelation>>();
            RelationTask relationTask {
                taskIndex,
                std::ref(tasks),
                std::ref(relations),
            };
            std::cout << "\r" << "Processing relations: " << count << "/" << total << " (" << percentage << "%)";
            boost::asio::post(pool, boost::bind(&Bootstrap::threadBootstrapRelationTask, this, relationTask));
        }

        pool.join();

        auto queries = allTasksQueries(tasks); // Get the queries
        for (auto it = queries->begin(); it != queries->end(); ++it) {
            db->query(*it);
        }
        lastid = relations->back().id;
        for (auto it = tasks->begin(); it != tasks->end(); ++it) {
            count += it->processed;
        }
    }
    percentage = (count * 100) / total;
    std::cout << "\r" << "Processing relations: " << count << "/" << total << " (" << percentage << "%)";

    std::cout << std::endl;

}

// This thread get started for every page of relation
void
Bootstrap::threadBootstrapRelationTask(RelationTask relationTask)
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("bootstrap::threadBootstrapRelationTask(relationTask): took %w seconds\n");
#endif
    auto taskIndex = relationTask.taskIndex;
    auto tasks = relationTask.tasks;
    auto relations = relationTask.relations;

    BootstrapTask task;
    int processed = 0;

    // Proccesing relations
    for (size_t i = taskIndex * page_size; i < (taskIndex + 1) * page_size; ++i) {
        if (i < relations->size()) {
            auto relation = relations->at(i);
            // Fill the rel_refs table
            for (auto mit = relation.members.begin(); mit != relation.members.end(); ++mit) {
                task.query.push_back("INSERT INTO rel_refs (rel_id, way_id) VALUES (" + std::to_string(relation.id) + "," + std::to_string(mit->ref) + ") ON CONFLICT DO NOTHING; ");
            }
            ++processed;
        }
    }
    task.processed = processed;
    const std::lock_guard<std::mutex> lock(tasks_change_mutex);
    (*tasks)[taskIndex] = task;

}

}
