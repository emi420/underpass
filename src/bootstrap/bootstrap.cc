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

#include <string.h>
#include "underpassconfig.hh"
#include "raw/queryraw.hh"
#include "data/pq.hh"
#include "bootstrap/bootstrap.hh"
#include <osmium/io/any_input.hpp>
#include <osmium/util/file.hpp>
#include <osmium/util/progress_bar.hpp>
#include <osmium/dynamic_handler.hpp>
#include <osmium/geom/factory.hpp>
#include <osmium/geom/wkt.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_manager.hpp>
#include "utils/log.hh"

using namespace queryraw;
using namespace underpassconfig;
using namespace logger;

using index_type = osmium::index::map::FlexMem<osmium::unsigned_object_id_type, osmium::Location>;
using location_handler_type = osmium::handler::NodeLocationsForWays<index_type>;

namespace bootstrap {

Bootstrap::Bootstrap(void) {}

// Handler to process nodes
class Handler : public osmium::handler::Handler {
public:

    Handler() {}

    std::vector<std::string> queries;
    std::shared_ptr<Pq> db;
    std::shared_ptr<QueryRaw> queryraw;
    osmium::ProgressBar* progress = nullptr;
    osmium::io::Reader* reader = nullptr;
    osmium::geom::WKTFactory<> factory;

    void node(const osmium::Node& node) {
        // std::cout << "Node " << node.id() << std::endl;

        OsmNode osmNode(node.location().lat(),  node.location().lon());
        osmNode.id = node.id();
        osmNode.version = node.version();
        // osmNode.timestamp = node.timestamp();
        for (const osmium::Tag& t : node.tags()) {
            osmNode.addTag(t.key(), t.value());
        }
        osmNode.action = osmobjects::create;
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

        if (progress && reader) {
            progress->update(reader->offset());
        }
    }

    void way(const osmium::Way& way) {
        // std::cout << "Way " << way.id() << std::endl;

        OsmWay osmWay;
        osmWay.id = way.id();
        osmWay.version = way.version();
        // osmWay.timestamp = way.timestamp;
        for (const auto& n : way.nodes()) {
            osmWay.refs.push_back(n.ref());
        }
        if (osmWay.isClosed()) {
            auto way_geom = factory.create_polygon(way);
            boost::geometry::read_wkt(way_geom, osmWay.polygon);
        } else {
            auto way_geom = factory.create_linestring(way);
            boost::geometry::read_wkt(way_geom, osmWay.linestring);
        }
        for (const osmium::Tag& t : way.tags()) {
            osmWay.addTag(t.key(), t.value());
        }
        osmWay.action = osmobjects::create;

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

        if (progress && reader) {
            progress->update(reader->offset());
        }
    }

};

class WKTDump : public osmium::handler::Handler {

    osmium::geom::WKTFactory<> m_factory;
    std::shared_ptr<Pq> db;
    std::shared_ptr<QueryRaw> queryraw;
    std::vector<std::string> queries;
    osmium::ProgressBar* progress = nullptr;
    osmium::io::Reader* reader = nullptr;


public:

    WKTDump(std::shared_ptr<Pq> m_db,
            std::shared_ptr<QueryRaw> m_queryraw,
            osmium::ProgressBar* m_progress,
            osmium::io::Reader* m_reader)
        : db(m_db), queryraw(m_queryraw), progress(m_progress), reader(m_reader) {}

    // This callback is called by osmium::apply for each area in the data.
    void area(const osmium::Area& area) {
        if (!area.from_way() && area.is_multipolygon()) {

            // auto relationId = area.orig_id()

            try {

                OsmRelation osmRelation;
                osmRelation.id = area.id();
                osmRelation.version = area.version();
                // osmRelation.timestamp = area.timestamp;

                for (const auto& member : area.members()) {
                //     if (member.ref() != 0) {
                //         osmRelation.addMember(
                //             member.ref(),
                //             member.type(),
                //             member.role()
                //         )
                //     }
                }

                for (const osmium::Tag& t : area.tags()) {
                    osmRelation.addTag(t.key(), t.value());
                }

                auto rel_wkt = m_factory.create_multipolygon(area);

                multipolygon_t rel_multipolygon;
                boost::geometry::read_wkt(rel_wkt, rel_multipolygon);

                polygon_t rel_polygon;
                boost::geometry::convert(rel_multipolygon[0], rel_polygon);

                std::stringstream ss;
                ss << std::setprecision(12) << boost::geometry::wkt(rel_polygon);
                std::string rel_polygon_wkt = ss.str();

                boost::geometry::read_wkt(rel_polygon_wkt, osmRelation.multipolygon);

                osmRelation.action = osmobjects::create;
                osmRelation.addTag("type", "multipolygon");

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

            } catch (const osmium::geometry_error& e) {
                std::cout << "GEOMETRY ERROR: " << e.what() << "\n";
            }
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
    pbf = config.import;

    if (pbf.empty()) {
        std::cout << "Usage: underpass --import <PBF file>" << std::endl;
        return;
    }
    processPBF();
}

void
Bootstrap::processPBF() {
    std::cout << "Reading PBF: " << pbf << std::endl;

    osmium::io::Reader reader{pbf};
    const osmium::io::File input_file{pbf};
    const osmium::area::Assembler::config_type assembler_config;
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config};
    osmium::relations::read_relations(input_file, mp_manager);

    osmium::ProgressBar progress{reader.file_size(), osmium::isatty(2)};
    auto queryraw = std::make_shared<QueryRaw>(db);

    // Nodes & ways
    index_type index;
    location_handler_type location_handler{index};
    Handler handler;
    handler.db = db;
    handler.progress = &progress;
    handler.reader = &reader;
    handler.queryraw = queryraw;
    std::cout << "Processing nodes and ways ..." << std::endl;
    osmium::apply(reader, location_handler, handler);

    progress.done();
    reader.close();

    // Relations
    osmium::io::Reader reader2{pbf};
    osmium::ProgressBar progress2{reader2.file_size(), osmium::isatty(2)};
    osmium::handler::DynamicHandler handler_rel;
    handler_rel.set<WKTDump>(db, queryraw, &progress2, &reader2);
    std::cout << "Processing relations ..." << std::endl;
    osmium::apply(reader2, location_handler, mp_manager.handler([&handler_rel](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, handler_rel);
    }));

    progress2.done();
    reader2.close();

}

}
