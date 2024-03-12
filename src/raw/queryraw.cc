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

/// \file queryraw.cc
/// \brief This file is used to work with the OSM Raw database
///
/// This manages the OSM Raw schema in a postgres database. This
/// includes querying existing data in the database, as well as
/// updating the database.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <iostream>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <map>
#include <string>
#include "utils/log.hh"
#include "data/pq.hh"
#include "raw/queryraw.hh"
#include "osm/osmobjects.hh"
#include "osm/osmchange.hh"

#include <boost/timer/timer.hpp>

using namespace pq;
using namespace logger;
using namespace osmobjects;
using namespace osmchange;

/// \namespace queryraw
namespace queryraw {

QueryRaw::QueryRaw(void) {}

QueryRaw::QueryRaw(std::shared_ptr<Pq> db) {
    dbconn = db;
}

std::string
QueryRaw::buildTagsQuery(std::map<std::string, std::string> tags) const {
    if (tags.size() > 0) {
        std::string tagsStr = "jsonb_build_object(";
        int count = 0;
        for (auto it = std::begin(tags); it != std::end(tags); ++it) {
            ++count;
            // PostgreSQL has an argument limit for functions
            if (count == 50) {
                tagsStr.erase(tagsStr.size() - 1);
                tagsStr += ") || jsonb_build_object(";
                count = 0;
            }
            std::string tag_format = "'%s', '%s',";
            boost::format tag_fmt(tag_format);
            tag_fmt % dbconn->escapedString(dbconn->escapedJSON(it->first));
            tag_fmt % dbconn->escapedString(dbconn->escapedJSON(it->second));
            tagsStr += tag_fmt.str();
        }
        tagsStr.erase(tagsStr.size() - 1);
        return tagsStr + ")";
    } else {
        return "null";
    }
}

std::string
buildMembersQuery(std::list<OsmRelationMember> members) {
    if (members.size() > 0) {
        std::string membersStr = "jsonb_build_array(";
        int count = 0;
        for (auto mit = std::begin(members); mit != std::end(members); ++mit) {
            membersStr += "jsonb_build_object(";
            std::string member_format = "'%s', '%s',";
            boost::format member_fmt(member_format);
            member_fmt % "role";
            member_fmt % mit->role;
            membersStr += member_fmt.str();
            member_fmt % "type";
            member_fmt % mit->type;
            membersStr += member_fmt.str();
            member_fmt % "ref";
            member_fmt % mit->ref;
            membersStr += member_fmt.str();
            membersStr.erase(membersStr.size() - 1);
            membersStr += "),";
        }
        membersStr.erase(membersStr.size() - 1);
        return membersStr += ")";
    } else {
        return "null";
    }
}

std::map<std::string, std::string> parseJSONObjectStr(std::string input) {
    std::map<std::string, std::string> obj;
    boost::property_tree::ptree pt;
    try {
        std::istringstream jsonStream(input);
        boost::property_tree::read_json(jsonStream, pt);
    } catch (const boost::property_tree::json_parser::json_parser_error& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return obj;
    }
    for (const auto& pair : pt) {
        obj[pair.first] = pair.second.get_value<std::string>();
    }
    return obj;
}

std::vector<std::map<std::string, std::string>> parseJSONArrayStr(std::string input) {
    std::vector<std::map<std::string, std::string>> arr;
    boost::property_tree::ptree pt;
    try {
        std::istringstream jsonStream(input);
        boost::property_tree::read_json(jsonStream, pt);
    } catch (const boost::property_tree::json_parser::json_parser_error& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return arr;
    }

    for (const auto& item : pt) {
        std::map<std::string, std::string> obj;
        for (const auto& pair : item.second) {
            obj[pair.first] = pair.second.get_value<std::string>();
        }
        arr.push_back(obj);
    }

    return arr;
}

std::string
QueryRaw::applyChange(const OsmNode &node) const
{
    std::string query;
    if (node.action == osmobjects::create || node.action == osmobjects::modify) {
        query = "INSERT INTO nodes as r (osm_id, geom, tags, timestamp, version, \"user\", uid, changeset) VALUES(";
        std::string format = "%d, ST_GeomFromText(\'%s\', 4326), %s, \'%s\', %d, \'%s\', %d, %d \
        ) ON CONFLICT (osm_id) DO UPDATE SET  geom = ST_GeomFromText(\'%s\', \
        4326), tags = %s, timestamp = \'%s\', version = %d, \"user\" = \'%s\', uid = %d, changeset = %d WHERE r.version < %d;";
        boost::format fmt(format);

        // osm_id
        fmt % node.id;

        // geometry
        std::stringstream ss;
        ss << std::setprecision(12) << boost::geometry::wkt(node.point);
        std::string geometry = ss.str();
        fmt % geometry;

        // tags
        auto tags = buildTagsQuery(node.tags);
        fmt % tags;
        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % node.version;
        // user
        fmt % dbconn->escapedString(node.user);
        // uid
        fmt % node.uid;
        // changeset
        fmt % node.changeset;

        // ON CONFLICT
        fmt % geometry;
        fmt % tags;
        fmt % timestamp;
        fmt % node.version;
        fmt % dbconn->escapedString(node.user);
        fmt % node.uid;
        fmt % node.changeset;
        fmt % node.version;

        query += fmt.str();

    } else if (node.action == osmobjects::remove) {
        query = "DELETE from nodes where osm_id = " + std::to_string(node.id) + ";";
    }

    return query;
}


const std::string QueryRaw::polyTable = "ways_poly";
const std::string QueryRaw::lineTable = "ways_line";

std::string
QueryRaw::applyChange(const OsmWay &way) const
{
    std::string query = "";
    const std::string* tableName;

    std::stringstream ss;
    if (way.refs.size() > 3 && (way.refs.front() == way.refs.back())) {
        tableName = &QueryRaw::polyTable;
        ss << std::setprecision(12) << boost::geometry::wkt(way.polygon);
    } else {
        tableName = &QueryRaw::lineTable;
        ss << std::setprecision(12) << boost::geometry::wkt(way.linestring);
    }
    std::string geostring = ss.str();

    if (way.refs.size() > 2
        && (way.action == osmobjects::create || way.action == osmobjects::modify)) {
        if ((way.refs.front() != way.refs.back() && way.refs.size() == boost::geometry::num_points(way.linestring)) ||
            (way.refs.front() == way.refs.back() && way.refs.size() == boost::geometry::num_points(way.polygon))
         ) {

            query = "INSERT INTO " + *tableName + " as r (osm_id, tags, refs, geom, timestamp, version, \"user\", uid, changeset) VALUES(";
            std::string format = "%d, %s, %s, %s, \'%s\', %d, \'%s\', %d, %d) \
            ON CONFLICT (osm_id) DO UPDATE SET tags = %s, refs = %s, geom = %s, timestamp = \'%s\', version = %d, \"user\" = \'%s\', uid = %d, changeset = %d WHERE r.version <= %d;";
            boost::format fmt(format);

            // osm_id
            fmt % way.id;

            //tags
            auto tags = buildTagsQuery(way.tags);
            fmt % tags;

            // refs
            std::string refs = "";
            for (auto it = std::begin(way.refs); it != std::end(way.refs); ++it) {
                refs += std::to_string(*it) + ",";
            }
            refs.erase(refs.size() - 1);
            refs = "ARRAY[" + refs + "]";
            fmt % refs;

            // geometry
            std::string geometry;
            geometry = "ST_GeomFromText(\'" + geostring + "\', 4326)";
            fmt % geometry;

            // timestamp
            std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
            fmt % timestamp;
            // version
            fmt % way.version;
            // user
            fmt % dbconn->escapedString(way.user);
            // uid
            fmt % way.uid;
            // changeset
            fmt % way.changeset;

            // ON CONFLICT
            fmt % tags;
            fmt % refs;
            fmt % geometry;
            fmt % timestamp;
            fmt % way.version;
            fmt % dbconn->escapedString(way.user);
            fmt % way.uid;
            fmt % way.changeset;
            fmt % way.version;

            query += fmt.str();

            query += "DELETE FROM way_refs WHERE way_id=" + std::to_string(way.id) + ";";
            for (auto ref = way.refs.begin(); ref != way.refs.end(); ++ref) {
                query += "INSERT INTO way_refs (way_id, node_id) VALUES (" + std::to_string(way.id) + "," + std::to_string(*ref) + ");";
            }
        }
    } else if (way.action == osmobjects::remove) {
        query += "DELETE FROM way_refs WHERE way_id=" + std::to_string(way.id) + ";";
        query += "DELETE FROM " + QueryRaw::polyTable + " where osm_id = " + std::to_string(way.id) + ";";
        query += "DELETE FROM " + QueryRaw::lineTable + " where osm_id = " + std::to_string(way.id) + ";";
    }

    return query;
}

std::string
QueryRaw::applyChange(const OsmRelation &relation) const
{
    std::string query = "";
    std::stringstream ss;
    if (relation.isMultiPolygon()) {
        ss << std::setprecision(12) << boost::geometry::wkt(relation.multipolygon);
    } else {
        ss << std::setprecision(12) << boost::geometry::wkt(relation.multilinestring);
    }
    
    std::string geostring = ss.str();

    if (relation.action == osmobjects::create || relation.action == osmobjects::modify) {

        query = "INSERT INTO relations as r (osm_id, tags, refs, geom, timestamp, version, \"user\", uid, changeset) VALUES(";
        std::string format = "%d, %s, %s, %s, \'%s\', %d, \'%s\', %d, %d) \
        ON CONFLICT (osm_id) DO UPDATE SET tags = %s, refs = %s, geom = %s, timestamp = \'%s\', version = %d, \"user\" = \'%s\', uid = %d, changeset = %d WHERE r.version <= %d;";
        boost::format fmt(format);

        // osm_id
        fmt % relation.id;

        // tags
        auto tags = buildTagsQuery(relation.tags);
        fmt % tags;

        // refs
        auto refs = buildMembersQuery(relation.members);
        fmt % refs;

        // geometry
        std::string geometry;
        geometry = "ST_GeomFromText(\'" + geostring + "\', 4326)";
        fmt % geometry;

        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % relation.version;
        // user
        fmt % dbconn->escapedString(relation.user);
        // uid
        fmt % relation.uid;
        // changeset
        fmt % relation.changeset;

        // ON CONFLICT
        fmt % tags;
        fmt % refs;
        fmt % geometry;
        fmt % timestamp;
        fmt % relation.version;
        fmt % dbconn->escapedString(relation.user);
        fmt % relation.uid;
        fmt % relation.changeset;
        fmt % relation.version;

        query += fmt.str();

        for (auto it = std::begin(relation.members); it != std::end(relation.members); ++it) {
            query += "INSERT INTO rel_refs (rel_id, way_id) VALUES (" + std::to_string(relation.id) + "," + std::to_string(it->ref) + ");";
        }

    } else if (relation.action == osmobjects::remove) {
        query += "DELETE FROM relations where osm_id = " + std::to_string(relation.id) + ";";
    }

    return query;}

std::vector<long> arrayStrToVector(std::string &refs_str) {
    refs_str.erase(0, 1);
    refs_str.erase(refs_str.size() - 1);
    std::vector<long> refs;
    std::stringstream ss(refs_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        refs.push_back(std::stod(token));
    }
    return refs;
}

std::list<std::shared_ptr<OsmRelation>>
QueryRaw::getRelationsByWaysRefs(std::string &wayIds) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getRelationsByWaysRefs(wayIds): took %w seconds\n");
#endif
    // Get all relations that have references to ways
    std::list<std::shared_ptr<osmobjects::OsmRelation>> rels;

    std::string relsQuery = "SELECT distinct(osm_id), refs, version, tags, uid, changeset from rel_refs join relations r on r.osm_id = rel_id where way_id = any(ARRAY[" + wayIds + "])";
    auto rels_result = dbconn->query(relsQuery);

    // Fill vector of OsmRelation objects
    for (auto rel_it = rels_result.begin(); rel_it != rels_result.end(); ++rel_it) {
        auto rel = std::make_shared<OsmRelation>();
        rel->id = (*rel_it)[0].as<long>();
        std::string refs_str = (*rel_it)[1].as<std::string>();
        auto members = parseJSONArrayStr(refs_str);
        for (auto mit = members.begin(); mit != members.end(); ++mit) {
            rel->addMember(std::stol(mit->at("ref")), osmobjects::osmtype_t::way, mit->at("role"));
        }
        rel->version = (*rel_it)[2].as<long>();
        auto tags = (*rel_it)[3];
        if (!tags.is_null()) {
            auto tags = parseJSONObjectStr((*rel_it)[3].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                rel->addTag(key, val);
            }
        }
        auto uid = (*rel_it)[4];
        if (!uid.is_null()) {
            rel->uid = (*rel_it)[4].as<long>();
        }
        auto changeset = (*rel_it)[5];
        if (!changeset.is_null()) {
            rel->changeset = (*rel_it)[5].as<long>();
        }
        rels.push_back(rel);
    }
    return rels;
}

void
QueryRaw::getWaysByIds(std::string &waysIds, std::map<long, std::shared_ptr<osmobjects::OsmWay>> &waycache) {
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getWaysByIds(waysIds, waycache): took %w seconds\n");
#endif
    // Get all ways that have references to nodes
    std::string waysQuery = "SELECT distinct(osm_id), ST_AsText(geom, 4326) from ways_poly wp where osm_id = any(ARRAY[" + waysIds + "])";
    auto ways_result = dbconn->query(waysQuery);

    // Fill vector of OsmWay objects
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        auto way = std::make_shared<OsmWay>();
        way->id = (*way_it)[0].as<long>();
        boost::geometry::read_wkt((*way_it)[1].as<std::string>(), way->polygon);
        waycache.insert(std::pair(way->id, std::make_shared<osmobjects::OsmWay>(*way)));
    }
}

// TODO: divide this function into multiple ones
void QueryRaw::buildGeometries(std::shared_ptr<OsmChangeFile> osmchanges, const multipolygon_t &poly)
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("buildGeometries(osmchanges, poly): took %w seconds\n");
#endif
    std::string referencedNodeIds;
    std::string modifiedNodesIds;
    std::string modifiedWaysIds;
    std::vector<long> removedWays;
    std::vector<long> removedRelations;

    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto wit = std::begin(change->ways); wit != std::end(change->ways); ++wit) {
            OsmWay *way = wit->get();
            if (way->action != osmobjects::remove) {
                // Save referenced nodes ids for later use
                for (auto rit = std::begin(way->refs); rit != std::end(way->refs); ++rit) {
                    if (!osmchanges->nodecache.count(*rit)) {
                        referencedNodeIds += std::to_string(*rit) + ",";
                    }
                }
                // Save ways for later use
                if (way->isClosed()) {
                    // Save only ways with a geometry that are inside the priority area
                    // these are mostly created ways
                    if (poly.empty() || boost::geometry::within(way->linestring, poly)) {
                        osmchanges->waycache.insert(std::make_pair(way->id, std::make_shared<osmobjects::OsmWay>(*way)));
                    }
                }
            } else {
                removedWays.push_back(way->id);
            }
        }

        // Save modified nodes for later use
        for (auto nit = std::begin(change->nodes); nit != std::end(change->nodes); ++nit) {
            OsmNode *node = nit->get();
            if (node->action == osmobjects::modify) {
                // Get only modified nodes ids inside the priority area
                if (poly.empty() || boost::geometry::within(node->point, poly)) {
                    modifiedNodesIds += std::to_string(node->id) + ",";
                }
            }
        }

        for (auto rel_it = std::begin(change->relations); rel_it != std::end(change->relations); ++rel_it) {
            OsmRelation *relation = rel_it->get();
            removedRelations.push_back(relation->id);
        }
    }

    // Add indirectly modified ways to osmchanges
    if (modifiedNodesIds.size() > 1) {
        modifiedNodesIds.erase(modifiedNodesIds.size() - 1);
        auto modifiedWays = getWaysByNodesRefs(modifiedNodesIds);
        auto change = std::make_shared<OsmChange>(none);
        for (auto wit = modifiedWays.begin(); wit != modifiedWays.end(); ++wit) {
           auto way = std::make_shared<OsmWay>(*wit->get());
           // Save referenced nodes for later use
           for (auto rit = std::begin(way->refs); rit != std::end(way->refs); ++rit) {
               if (!osmchanges->nodecache.count(*rit)) {
                   referencedNodeIds += std::to_string(*rit) + ",";
               }
           }
           // If the way is not marked as removed, mark it as modified
           if (std::find(removedWays.begin(), removedWays.end(), way->id) == removedWays.end()) {
                way->action = osmobjects::modify;
                change->ways.push_back(way);
                modifiedWaysIds += std::to_string(way->id) + ",";
           }
        }
        osmchanges->changes.push_back(change);
    }

    // Add indirectly modified relations to osmchanges
    if (modifiedWaysIds.size() > 1) {
        modifiedWaysIds.erase(modifiedWaysIds.size() - 1);
        auto modifiedRelations = getRelationsByWaysRefs(modifiedWaysIds);
        auto change = std::make_shared<OsmChange>(none);
        for (auto rel_it = modifiedRelations.begin(); rel_it != modifiedRelations.end(); ++rel_it) {
           auto relation = std::make_shared<OsmRelation>(*rel_it->get());
           // If the relation is not marked as removed, mark it as modified
           if (std::find(removedRelations.begin(), removedRelations.end(), relation->id) == removedRelations.end()) {
                relation->action = osmobjects::modify;
                change->relations.push_back(relation);

           }
        }
        osmchanges->changes.push_back(change);
    }

    // Fill nodecache with referenced nodes
    if (referencedNodeIds.size() > 1) {
        referencedNodeIds.erase(referencedNodeIds.size() - 1);
        // Get Nodes from DB
        std::string nodesQuery = "SELECT osm_id, st_x(geom) as lat, st_y(geom) as lon FROM nodes where osm_id in (" + referencedNodeIds + ");";
        auto result = dbconn->query(nodesQuery);
        // Fill nodecache
        for (auto node_it = result.begin(); node_it != result.end(); ++node_it) {
            auto node_id = (*node_it)[0].as<long>();
            auto node_lat = (*node_it)[2].as<double>();
            auto node_lon = (*node_it)[1].as<double>();
            OsmNode node(node_lat, node_lon);
            osmchanges->nodecache[node_id] = node.point;
        }
    }

    // Build ways geometries using nodecache
    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto wit = std::begin(change->ways); wit != std::end(change->ways); ++wit) {
            OsmWay *way = wit->get();
            way->linestring.clear();
            for (auto rit = way->refs.begin(); rit != way->refs.end(); ++rit) {
                if (osmchanges->nodecache.count(*rit)) {
                    boost::geometry::append(way->linestring, osmchanges->nodecache.at(*rit));
                }
            }
            if (way->isClosed()) {
                way->polygon = { {std::begin(way->linestring), std::end(way->linestring)} };
            }
            // Save way pointer for later use
            if (poly.empty() || boost::geometry::within(way->linestring, poly)) {
                if (osmchanges->waycache.count(way->id)) {
                    osmchanges->waycache.at(way->id)->polygon = way->polygon;
                } else {
                    osmchanges->waycache.insert(std::make_pair(way->id, std::make_shared<osmobjects::OsmWay>(*way)));
                }
            }
        }
    }

    // Filter out all relations that doesn't have at least 1 way in cache
    std::string relsForWayCacheIds;
    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto rel_it = std::begin(change->relations); rel_it != std::end(change->relations); ++rel_it) {
            OsmRelation *relation = rel_it->get();
            if (relation->isMultiPolygon() || relation->isMultiLineString()) {
                bool getWaysForRelation = false;
                for (auto mit = relation->members.begin(); mit != relation->members.end(); ++mit) {
                    if (osmchanges->waycache.count(mit->ref)) {
                        getWaysForRelation = true;
                        break;
                    }
                }
                if (getWaysForRelation) {
                    relation->priority = true;
                    for (auto mit = relation->members.begin(); mit != relation->members.end(); ++mit) {
                        if (!osmchanges->waycache.count(mit->ref)) {
                           relsForWayCacheIds += std::to_string(mit->ref) + ",";
                        }
                    }
                } else {
                    relation->priority = false;
                }
            }
        }
    }
    // Get all missing ways geometries for relations
    if (relsForWayCacheIds != "") {
        relsForWayCacheIds.erase(relsForWayCacheIds.size() - 1);
        getWaysByIds(relsForWayCacheIds, osmchanges->waycache);
    }

    // Build relation geometries
    for (auto it = std::begin(osmchanges->changes); it != std::end(osmchanges->changes); it++) {
        OsmChange *change = it->get();
        for (auto rel_it = std::begin(change->relations); rel_it != std::end(change->relations); ++rel_it) {
            OsmRelation *relation = rel_it->get();
            if (relation->priority) {
                osmchanges->buildRelationGeometry(*relation);
            }
        }
    }
}

void
QueryRaw::getNodeCacheFromWays(std::shared_ptr<std::vector<OsmWay>> ways, std::map<double, point_t> &nodecache) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getNodeCacheFromWays(ways, nodecache): took %w seconds\n");
#endif

    // Get all nodes ids referenced in ways
    std::string nodeIds;
    for (auto wit = ways->begin(); wit != ways->end(); ++wit) {
        for (auto rit = std::begin(wit->refs); rit != std::end(wit->refs); ++rit) {
            nodeIds += std::to_string(*rit) + ",";
        }
    }
    if (nodeIds.size() > 1) {

        nodeIds.erase(nodeIds.size() - 1);

        // Get Nodes from DB
        std::string nodesQuery = "SELECT osm_id, st_x(geom) as lat, st_y(geom) as lon FROM nodes where osm_id in (" + nodeIds + ") and st_x(geom) is not null and st_y(geom) is not null;";
        auto result = dbconn->query(nodesQuery);
        // Fill nodecache
        for (auto node_it = result.begin(); node_it != result.end(); ++node_it) {
            auto node_id = (*node_it)[0].as<long>();
            auto node_lat = (*node_it)[1].as<double>();
            auto node_lon = (*node_it)[2].as<double>();
            auto point = point_t(node_lat, node_lon);
            nodecache.insert(std::make_pair(node_id, point));
        }
    }
}

std::list<std::shared_ptr<OsmWay>>
QueryRaw::getWaysByNodesRefs(std::string &nodeIds) const
{
#ifdef TIMING_DEBUG
    boost::timer::auto_cpu_timer timer("getWaysByNodesRefs(nodeIds): took %w seconds\n");
#endif
    // Get all ways that have references to nodes
    std::list<std::shared_ptr<osmobjects::OsmWay>> ways;

    std::string waysQuery = "SELECT distinct(osm_id), refs, version, tags, uid, changeset from way_refs join ways_poly wp on wp.osm_id = way_id where node_id = any(ARRAY[" + nodeIds + "])";
    waysQuery += " UNION SELECT distinct(osm_id), refs, version, tags, uid, changeset from way_refs join ways_line wl on wl.osm_id = way_id where node_id = any(ARRAY[" + nodeIds + "]);";
    auto ways_result = dbconn->query(waysQuery);

    // Fill vector of OsmWay objects
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        auto way = std::make_shared<OsmWay>();
        way->id = (*way_it)[0].as<long>();
        std::string refs_str = (*way_it)[1].as<std::string>();
        if (refs_str.size() > 1) {
            way->refs = arrayStrToVector(refs_str);
        }
        way->version = (*way_it)[2].as<long>();
        auto tags = (*way_it)[3];
        if (!tags.is_null()) {
            auto tags = parseJSONObjectStr((*way_it)[3].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                way->addTag(key, val);
            }
        }
        auto uid = (*way_it)[4];
        if (!uid.is_null()) {
            way->uid = (*way_it)[4].as<long>();
        }
        auto changeset = (*way_it)[5];
        if (!changeset.is_null()) {
            way->changeset = (*way_it)[5].as<long>();
        }
        ways.push_back(way);
    }
    return ways;
}

int QueryRaw::getCount(const std::string &tableName) {
    std::string query = "select count(osm_id) from " + tableName;
    auto result = dbconn->query(query);
    return result[0][0].as<int>();
}

std::shared_ptr<std::vector<OsmNode>>
QueryRaw::getNodesFromDB(long lastid, int pageSize) {
    std::string nodesQuery = "SELECT osm_id, ST_AsText(geom, 4326)";

    if (lastid > 0) {
        nodesQuery += ", version, tags FROM nodes where osm_id < " + std::to_string(lastid) + " order by osm_id desc limit " + std::to_string(pageSize) + ";";
    } else {
        nodesQuery += ", version, tags FROM nodes order by osm_id desc limit " + std::to_string(pageSize) + ";";
    }

    auto nodes_result = dbconn->query(nodesQuery);
    // Fill vector of OsmNode objects
    auto nodes = std::make_shared<std::vector<OsmNode>>();
    for (auto node_it = nodes_result.begin(); node_it != nodes_result.end(); ++node_it) {
        OsmNode node;
        node.id = (*node_it)[0].as<long>();

        point_t point;
        std::string point_str = (*node_it)[1].as<std::string>();
        boost::geometry::read_wkt(point_str, point);
        node.setPoint(boost::geometry::get<0>(point), boost::geometry::get<1>(point));
        node.version = (*node_it)[2].as<long>();
        auto tags = (*node_it)[3];
        if (!tags.is_null()) {
            auto tags = parseJSONObjectStr((*node_it)[3].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                node.addTag(key, val);
            }
        }
        nodes->push_back(node);
    }

    return nodes;
}


std::shared_ptr<std::vector<OsmWay>>
QueryRaw::getWaysFromDB(long lastid, int pageSize, const std::string &tableName) {
    std::string waysQuery;
    if (tableName == QueryRaw::polyTable) {
        waysQuery = "SELECT osm_id, refs, ST_AsText(ST_ExteriorRing(geom), 4326)";
    } else {
        waysQuery = "SELECT osm_id, refs, ST_AsText(geom, 4326)";
    }
    if (lastid > 0) {
        waysQuery += ", version, tags FROM " + tableName + " where osm_id < " + std::to_string(lastid) + " order by osm_id desc limit " + std::to_string(pageSize) + ";";
    } else {
        waysQuery += ", version, tags FROM " + tableName + " order by osm_id desc limit " + std::to_string(pageSize) + ";";
    }

    auto ways_result = dbconn->query(waysQuery);
    // Fill vector of OsmWay objects
    auto ways = std::make_shared<std::vector<OsmWay>>();
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        OsmWay way;
        way.id = (*way_it)[0].as<long>();
        std::string refs_str = (*way_it)[1].as<std::string>();
        if (refs_str.size() > 1) {
            way.refs = arrayStrToVector(refs_str);

            std::string poly = (*way_it)[2].as<std::string>();
            boost::geometry::read_wkt(poly, way.linestring);

            if (tableName == QueryRaw::polyTable) {
                way.polygon = { {std::begin(way.linestring), std::end(way.linestring)} };
            }
            way.version = (*way_it)[3].as<long>();
            auto tags = (*way_it)[4];
            if (!tags.is_null()) {
                auto tags = parseJSONObjectStr((*way_it)[4].as<std::string>());
                for (auto const& [key, val] : tags)
                {
                    way.addTag(key, val);
                }
            }
            ways->push_back(way);
        }
    }

    return ways;
}

std::shared_ptr<std::vector<OsmWay>>
QueryRaw::getWaysFromDBWithoutRefs(long lastid, int pageSize, const std::string &tableName) {
    std::string waysQuery;
    if (tableName == QueryRaw::polyTable) {
        waysQuery = "SELECT osm_id, ST_AsText(ST_ExteriorRing(geom), 4326)";
    } else {
        waysQuery = "SELECT osm_id, ST_AsText(geom, 4326)";
    }
    if (lastid > 0) {
        waysQuery += ", tags FROM " + tableName + " where osm_id < " + std::to_string(lastid) + " order by osm_id desc limit " + std::to_string(pageSize) + ";";
    } else {
        waysQuery += ", tags FROM " + tableName + " order by osm_id desc limit " + std::to_string(pageSize) + ";";
    }

    auto ways_result = dbconn->query(waysQuery);
    // Fill vector of OsmWay objects
    auto ways = std::make_shared<std::vector<OsmWay>>();
    for (auto way_it = ways_result.begin(); way_it != ways_result.end(); ++way_it) {
        OsmWay way;
        way.id = (*way_it)[0].as<long>();

        std::string poly = (*way_it)[1].as<std::string>();
        boost::geometry::read_wkt(poly, way.linestring);

        if (tableName == QueryRaw::polyTable) {
            way.polygon = { {std::begin(way.linestring), std::end(way.linestring)} };
        }
        auto tags = (*way_it)[2];
        if (!tags.is_null()) {
            auto tags = parseJSONObjectStr((*way_it)[2].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                way.addTag(key, val);
            }
        }
        ways->push_back(way);

    }

    return ways;
}

std::shared_ptr<std::vector<OsmRelation>>
QueryRaw::getRelationsFromDB(long lastid, int pageSize) {
    std::string relationsQuery = "SELECT osm_id, refs, ST_AsText(geom, 4326)";
    if (lastid > 0) {
        relationsQuery += ", version, tags FROM relations where osm_id < " + std::to_string(lastid) + " order by osm_id desc limit " + std::to_string(pageSize) + ";";
    } else {
        relationsQuery += ", version, tags FROM relations order by osm_id desc limit " + std::to_string(pageSize) + ";";
    }

    auto relations_result = dbconn->query(relationsQuery);
    // Fill vector of OsmRelation objects
    auto relations = std::make_shared<std::vector<OsmRelation>>();
    for (auto rel_it = relations_result.begin(); rel_it != relations_result.end(); ++rel_it) {
        OsmRelation relation;
        relation.id = (*rel_it)[0].as<long>();
        auto refs = (*rel_it)[1];
        if (!refs.is_null()) {
            auto refs = parseJSONArrayStr((*rel_it)[1].as<std::string>());
            for (auto ref_it = refs.begin(); ref_it != refs.end(); ++ref_it) {
                if (ref_it->at("type") == "w" && (ref_it->at("role") == "inner" || ref_it->at("role") == "outer")) {
                    relation.addMember(
                        std::stoi(ref_it->at("ref")),
                        osmobjects::osmtype_t::way,
                        ref_it->at("role")
                    );
                }
            }
            std::string geometry = (*rel_it)[2].as<std::string>();
            if (geometry.substr(0, 12) == "MULTIPOLYGON") {
                boost::geometry::read_wkt(geometry, relation.multipolygon);
            } else if (geometry.substr(0, 15) == "MULTILINESTRING") {
                boost::geometry::read_wkt(geometry, relation.multilinestring);
            }
            relation.version = (*rel_it)[3].as<long>();
        }
        auto tags = (*rel_it)[4];
        if (!tags.is_null()) {
            auto tags = parseJSONObjectStr((*rel_it)[4].as<std::string>());
            for (auto const& [key, val] : tags)
            {
                relation.addTag(key, val);
            }
        }
        relations->push_back(relation);
    }

    return relations;
}


} // namespace queryraw

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
