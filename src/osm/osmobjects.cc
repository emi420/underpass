//
// Copyright (c) 2020, 2021, 2023 Humanitarian OpenStreetMap Team
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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
//#include <pqxx/pqxx>
#ifdef LIBXML
#include <libxml++/libxml++.h>
#endif
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1
// #include <boost/geometry.hpp>
// typedef boost::geometry::model::d2::point_xy<double> point_t;
// typedef boost::geometry::model::polygon<point_t> polygon_t;
// typedef boost::geometry::model::multi_polygon<polygon_t> multipolygon_t;
// typedef boost::geometry::model::linestring<point_t> linestring_t;

#include "osm/osmobjects.hh"

#include "utils/log.hh"
using namespace logger;

namespace osmobjects {

void
OsmObject::dump(void) const
{
    std::cerr << "Dumping OsmObject()" << std::endl;
    if (action == create) {
        std::cerr << "\tAction: Create" << std::endl;
    } else if (action == modify) {
        std::cerr << "\tAction: Modify" << std::endl;
    } else if (action == remove) {
        std::cerr << "\tAction: Delete" << std::endl;
    }

    if (type == node) {
        std::cerr << "\tType: OsmNode" << std::endl;
    } else if (type == way) {
        std::cerr << "\tType: OsmWay" << std::endl;
    } else if (type == relation) {
        std::cerr << "\tType: OsmRelation" << std::endl;
    }
    std::cerr << "\tID: " << std::to_string(id) << std::endl;
    std::cerr << "\tVersion: " << std::to_string(version) << std::endl;
    std::cerr << "\tTimestamp: " << to_simple_string(timestamp) << std::endl;
    std::cerr << "\tUID: " << std::to_string(uid) << std::endl;

    std::cerr << "\tUser: " << user << std::endl;
    if (priority) {
        std::cerr << "\tIn Priority area" << std::endl;
    } else {
        std::cerr << "\tNot in Priority area" << std::endl;
    }
    if (change_id > 0) {
        std::cerr << "\tChange ID: " << std::to_string(change_id) << std::endl;
    }
    if (tags.size() > 0) {
        std::cerr << "\tTags: " << tags.size() << std::endl;
        for (auto it = std::begin(tags); it != std::end(tags); ++it) {
            std::cerr << "\t\t" << it->first << ":" << it->second << std::endl;
        }
    }
};

// This represents an ODM node. A node has point coordinates, and may
// contain tags if it's a POI.
// void
// OsmWay::makeLinestring(point_t point)
// {
//     // If the first and last ref are the same, it's a closed polygon,
//     // like a building.
//     if (refs.begin() == refs.end()) {
//         boost::geometry::append(polygon, point);
//     } else {
//         boost::geometry::append(linestring, point);
//     }
// };

void
OsmWay::dump(void) const
{
    OsmObject::dump();
    if (refs.size() > 0) {
        std::cerr << "\tRefs: " << refs.size() << std::endl;
        std::string tmp;
        for (auto it = std::begin(refs); it != std::end(refs); ++it) {
            tmp += std::to_string(*it) + ", ";
        }
        tmp.pop_back();
        tmp.pop_back();
        std::cerr << "\t" << tmp << std::endl;
    }
    std::cerr << boost::geometry::wkt(linestring) << std::endl;
    std::cerr << boost::geometry::wkt(polygon) << std::endl;
    if (tags.size() > 0) {
        std::cerr << "\tTags: " << tags.size() << std::endl;
        for (auto const& [key, val] : tags)
        {
            std::cerr << key
                    << ':'
                    << val
                    << ", ";
        }
        std::cerr << std::endl;
    } else {
        std::cerr << "No tags." << std::endl;
    }
};

void
OsmRelation::dump() const
{
    OsmObject::dump();
    if (members.size() > 0) {
        std::cerr << "\tMembers: " << members.size() << std::endl;
        std::string tmp;
        for (const auto &member: std::as_const(members)) {
            member.dump();
        }
        std::cerr << "\t" << tmp << std::endl;
    }
}

void
OsmRelationMember::dump() const
{
    std::cerr << "\t\tDumping Relation member" << std::endl;
    std::cerr << "\t\t\tRef: " << ref << std::endl;
    std::cerr << "\t\t\type: " << type << std::endl;
    std::cerr << "\t\t\tRole: " << role << std::endl;
}

int
OsmNode::getZ_order() const
{
    return z_order;
}

void
OsmNode::setZ_order(int newZ_order)
{
    z_order = newZ_order;
}

} // namespace osmobjects
