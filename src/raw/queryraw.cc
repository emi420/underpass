//
// Copyright (c) 2023 Humanitarian OpenStreetMap Team
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
#include <map>
#include <string>
#include "utils/log.hh"
#include "data/pq.hh"
#include "raw/queryraw.hh"
#include "osm/osmobjects.hh"

using namespace pq;
using namespace logger;
using namespace osmobjects;

/// \namespace queryraw
namespace queryraw {

QueryRaw::QueryRaw(void) {}

QueryRaw::QueryRaw(std::shared_ptr<Pq> db) {
    dbconn = db;
}

std::string
QueryRaw::applyChange(const OsmNode &node) const
{
#ifdef TIMING_DEBUG_X
    boost::timer::auto_cpu_timer timer("applyChange(raw node): took %w seconds\n");
#endif
    std::string query;
    if (node.action == osmobjects::create || node.action == osmobjects::modify) {
        query = "INSERT INTO raw (osm_id, change_id, osm_type, geometry, tags, timestamp, version) VALUES(";
        std::string format = "%d, %d, 'N', ST_GeomFromText(\'%s\', 4326), %s, \'%s\', %d \
        ) ON CONFLICT (osm_id) DO UPDATE SET change_id = %d, geometry = ST_GeomFromText(\'%s\', \
        4326), tags = %s, timestamp = \'%s\', version = %d WHERE excluded.version < %d;";
        boost::format fmt(format);

        // osm_id
        fmt % node.id;
        // change_id
        fmt % node.change_id;
        // geometry
        auto geometry = boost::geometry::wkt(node.point);
        fmt % geometry;
        // tags
        std::string tags = "";
        if (node.tags.size() > 0) {
            for (auto it = std::begin(node.tags); it != std::end(node.tags); ++it) {
                std::string tag_format = "\"%s\" => \"%s\",";
                boost::format tag_fmt(tag_format);
                tag_fmt % dbconn->escapedString(it->first);
                tag_fmt % dbconn->escapedString(it->second);
                tags += tag_fmt.str();
            }
            tags.erase(tags.size() - 1);
            tags = "'" + tags + "'";
        } else {
            tags = "null";
        }
        fmt % tags;
        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % node.version;

        // ON CONFLICT
        fmt % node.change_id;
        fmt % geometry;
        fmt % tags;
        fmt % timestamp;
        fmt % node.version;
        fmt % node.version;

        query += fmt.str();

    } else if (node.action == osmobjects::remove) {
        query = "DELETE from raw where osm_id = " + std::to_string(node.id) + ";";
    }

    std::cout << "[QUERY] " << query << std::endl << std::endl;
    return query;
}

std::string
QueryRaw::applyChange(const OsmWay &way) const
{
#ifdef TIMING_DEBUG_X
    boost::timer::auto_cpu_timer timer("applyChange(raw way): took %w seconds\n");
#endif
    std::string query = "";
    if (way.action == osmobjects::create || way.action == osmobjects::modify) {
        query = "INSERT INTO raw (osm_id, change_id, osm_type, geometry, tags, refs, timestamp, version) VALUES(";
        std::string format = "%d, %d, \'%s\', %s, %s, %s, \'%s\') \
        ON CONFLICT (osm_id) DO UPDATE SET change_id = %d, geometry = %s, tags = %s, refs = %s, timestamp = \'%s\', version = %d WHERE version < %d;";
        boost::format fmt(format);
        // osm_id
        fmt % way.id;
        // change_id
        fmt % way.change_id;
        // osm_type
        fmt % "W";

        // geometry (not used yet)
        std::string geometry = "null";
        // if (boost::geometry::num_points(way.linestring) > 0) {
        //     std::string geostring = boost::lexical_cast<std::string>(boost::geometry::wkt(way.linestring));
        //     geometry = "ST_GeomFromText(\'" + geostring + "\', 4326)";
        // } else if (boost::geometry::num_points(way.polygon) > 0) {
        //     std::string geostring = boost::lexical_cast<std::string>(boost::geometry::wkt(way.polygon));
        //     geometry = "ST_GeomFromText(\'" + geostring + "\', 4326)";
        // } else {
        //     geometry = "null";
        // }
        fmt % geometry;

        // tags
        std::string tags = "";
        if (way.tags.size() > 0) {
            for (auto it = std::begin(way.tags); it != std::end(way.tags); ++it) {
                std::string tag_format = "\"%s\" => \"%s\",";
                boost::format tag_fmt(tag_format);
                tag_fmt % dbconn->escapedString(it->first);
                tag_fmt % dbconn->escapedString(it->second);
                tags += tag_fmt.str();
            }
            tags.erase(tags.size() - 1);
            tags = "'" + tags + "'";
        } else {
            tags = "null";
        }

        fmt % tags;
        // refs
        std::string refs = "";
        if (way.refs.size() > 0) {
            for (auto it = std::begin(way.refs); it != std::end(way.refs); ++it) {
                refs += std::to_string(*it) + ",";
            }
            refs.erase(refs.size() - 1);
            refs = "ARRAY[" + refs + "]";
        } else {
            refs = "null";
        }
        fmt % refs;
        // timestamp
        std::string timestamp = to_simple_string(boost::posix_time::microsec_clock::universal_time());
        fmt % timestamp;
        // version
        fmt % way.version;

        // ON CONFLICT
        fmt % way.change_id;
        fmt % geometry;
        fmt % tags;
        fmt % refs;
        fmt % timestamp;
        fmt % way.version;
        fmt % way.version;

        query += fmt.str();

    } else if (way.action == osmobjects::remove) {
        query = "DELETE from raw where osm_id = " + std::to_string(way.id) + ";";
    }
    
    return query;
}

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

std::string
QueryRaw::applyChange(const std::shared_ptr<std::map<long, std::pair<double, double>>> nodes) const
{
#ifdef TIMING_DEBUG_X
    boost::timer::auto_cpu_timer timer("applyChange(modified nodes): took %w seconds\n");
#endif
    // 1. Get all ways that have references to nodes
    std::string nodeIds;
    for (auto node = nodes->begin(); node != nodes->end(); ++node) {
        nodeIds += std::to_string(node->first) + ",";
    }
    nodeIds.erase(nodeIds.size() - 1);
    std::string waysQuery = "SELECT osm_id, refs FROM raw where refs && ARRAY[" + nodeIds + "]::bigint[];";
    auto ways = dbconn->query(waysQuery);

    // 2. Update ways geometries

    // For each way in results ...
    std::string query = "";
    for (auto way_it = ways.begin(); way_it != ways.end(); ++way_it) {
        std::string queryPoints = "";
        std::string queryWay = "";
        // Way OSM id
        auto osm_id = (*way_it)[0].as<long>();
        // Way refs
        std::string refs_str = (*way_it)[1].as<std::string>();
        // For each way ref ...
        if (refs_str.size() > 1) {
            auto refs =  arrayStrToVector(refs_str);
            // For each ref in way ...
            int refIndex = 0;
            queryWay += "UPDATE raw SET geometry = ";
            for (auto ref_it = refs.begin(); ref_it != refs.end(); ++ref_it) {
                // If node was modified, update it
                if (nodes->find(*ref_it) != nodes->end()) {
                    queryWay += "ST_SetPoint(";
                    queryPoints += std::to_string(refIndex) + ", ST_MakePoint(";
                    queryPoints += std::to_string(nodes->at(*ref_it).first) + "," + std::to_string(nodes->at(*ref_it).second);
                    queryPoints += ")),";
                }
                refIndex++;
            }
        }
        queryPoints.erase(queryPoints.size() - 1);
        queryWay += "geometry, " + queryPoints;
        queryWay += " WHERE osm_id = " + std::to_string(osm_id) + ";";
        query += queryWay;

    }
    // 3. Save ways
    return query;
}

} // namespace queryraw

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
