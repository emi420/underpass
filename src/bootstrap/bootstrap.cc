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

class RelationHandler : public osmium::relations::RelationsManager<RelationHandler, false, true, false> {

public:

    osmium::geom::WKTFactory<> m_factory;
    std::shared_ptr<Pq> db;
    std::shared_ptr<QueryRaw> queryraw;
    std::vector<std::string> queries;
    std::map<long, std::shared_ptr<osmobjects::OsmRelation>>* relations = nullptr;
    osmium::ProgressBar* progress = nullptr;
    osmium::io::Reader* reader = nullptr;

    RelationHandler(
        std::shared_ptr<Pq> m_db,
        std::shared_ptr<QueryRaw> m_queryraw,
        std::map<long, std::shared_ptr<osmobjects::OsmRelation>>* m_relations,
        osmium::ProgressBar* m_progress,
        osmium::io::Reader* m_reader)
    : db(m_db), queryraw(m_queryraw), relations(m_relations), progress(m_progress), reader(m_reader) {}

    // Callback for newly found relations (first pass)
    bool new_relation(const osmium::Relation& relation) noexcept {
        return relation.tags().has_tag("type", "multipolygon") || relation.tags().has_tag("type", "boundary");
    }

    // Callback for completed relations (second pass)
    void complete_relation(const osmium::Relation& relation) {

        OsmRelation osmRelation;

        // osm_id
        osmRelation.id = relation.id();

        // tags
        for (const osmium::Tag& t : relation.tags()) {
            osmRelation.addTag(t.key(), t.value());
        }

        // refs
        for (const auto& member : relation.members()) {
            if (member.ref() != 0) {
                std::ostringstream oss;
                oss << member.type();
                std::string member_type = oss.str();
                osmobjects::osmtype_t obj_type = osmobjects::osmtype_t::empty;

                if (member_type == "w") {
                    obj_type = osmobjects::osmtype_t::way;
                } else if (member_type == "n") {
                    obj_type = osmobjects::osmtype_t::node;
                } else if (member_type == "r") {
                    obj_type = osmobjects::osmtype_t::relation;
                }
                osmRelation.addMember(
                    member.ref(),
                    obj_type,
                    member.role()
                );
            }
        }

        // version
        osmRelation.version = relation.version();

        // action
        osmRelation.action = osmobjects::create;

        // Save relation for complete it later
        relations->insert(std::pair(osmRelation.id, std::make_shared<osmobjects::OsmRelation>(osmRelation)));

        if (progress && reader) {
            progress->update(reader->offset());
        }

    }

};

// Handler to process nodes and ways
class NodeWayHandler : public osmium::handler::Handler {
public:

    NodeWayHandler() {}

    std::vector<std::string> queries;
    std::shared_ptr<Pq> db;
    std::shared_ptr<QueryRaw> queryraw;
    osmium::ProgressBar* progress = nullptr;
    osmium::io::Reader* reader = nullptr;
    osmium::geom::WKTFactory<> factory;

    void node(const osmium::Node& node) {
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

class RelationGeometryHandler : public osmium::handler::Handler {

    osmium::geom::WKTFactory<> m_factory;
    std::shared_ptr<Pq> db;
    std::shared_ptr<QueryRaw> queryraw;
    std::vector<std::string> queries;
    std::map<long, std::shared_ptr<osmobjects::OsmRelation>>* relations = nullptr;
    osmium::ProgressBar* progress = nullptr;
    osmium::io::Reader* reader = nullptr;

public:

    RelationGeometryHandler(std::shared_ptr<Pq> m_db,
            std::shared_ptr<QueryRaw> m_queryraw,
            std::map<long, std::shared_ptr<osmobjects::OsmRelation>>* m_relations,
            osmium::ProgressBar* m_progress,
            osmium::io::Reader* m_reader)
        : db(m_db), queryraw(m_queryraw), relations(m_relations), progress(m_progress), reader(m_reader) {}

    // This callback is called by osmium::apply for each area in the data.
    void area(const osmium::Area& area) {

        try {

            if (!area.from_way()) {

                auto osmRelation = relations->at(area.orig_id());

                auto rel_wkt = m_factory.create_multipolygon(area);

                multipolygon_t rel_multipolygon;
                boost::geometry::read_wkt(rel_wkt, rel_multipolygon);

                polygon_t rel_polygon;
                boost::geometry::convert(rel_multipolygon[0], rel_polygon);

                std::stringstream ss;
                ss << std::setprecision(12) << boost::geometry::wkt(rel_polygon);
                std::string rel_polygon_wkt = ss.str();

                boost::geometry::read_wkt(rel_polygon_wkt, osmRelation->multipolygon);

                std::shared_ptr<std::vector<std::string>> query = queryraw->applyChange(*osmRelation);

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

        } catch (const osmium::geometry_error& e) {
            std::cout << "GEOMETRY ERROR: " << e.what() << "\n";
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

    osmium::ProgressBar progress{reader.file_size(), osmium::isatty(2)};
    auto queryraw = std::make_shared<QueryRaw>(db);

    // Nodes & ways
    std::cout << "Processing nodes and ways ..." << std::endl;
    index_type index;
    location_handler_type location_handler{index};
    NodeWayHandler handler;
    handler.db = db;
    handler.progress = &progress;
    handler.reader = &reader;
    handler.queryraw = queryraw;
    osmium::apply(reader, location_handler, handler);
    progress.done();


    std::map<long, std::shared_ptr<osmobjects::OsmRelation>> relations;

    // Relations
    std::cout << "Processing relations  ..." << std::endl;
    osmium::io::Reader reader3{input_file};
    osmium::ProgressBar progress3{reader3.file_size(), osmium::isatty(2)};
    RelationHandler relationHandler(db, queryraw, &relations, &progress3, &reader3);
    osmium::relations::read_relations(input_file, relationHandler);
    osmium::apply(reader3, relationHandler.handler());
    osmium::memory::Buffer buffer = relationHandler.read();
    progress3.done();

    // Relations geometries
    std::cout << "Processing relations (add geometries) ..." << std::endl;
    osmium::area::MultipolygonManager<osmium::area::Assembler> mp_manager{assembler_config};
    osmium::relations::read_relations(input_file, mp_manager);
    osmium::io::Reader reader2{pbf};
    osmium::ProgressBar progress2{reader2.file_size(), osmium::isatty(2)};
    osmium::handler::DynamicHandler dynamicHandler;
    dynamicHandler.set<RelationGeometryHandler>(db, queryraw, &relations, &progress2, &reader2);
    osmium::apply(reader2, location_handler, mp_manager.handler([&dynamicHandler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, dynamicHandler);
    }));
    progress2.done();

}

}
