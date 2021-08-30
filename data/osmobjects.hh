//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
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

#ifndef __OSMOBJECTS_HH__
#define __OSMOBJECTS_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <iostream>
#include <boost/geometry.hpp>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1
#include <boost/progress.hpp>

typedef boost::geometry::model::d2::point_xy<double> point_t;
typedef boost::geometry::model::polygon<point_t> polygon_t;
typedef boost::geometry::model::multi_polygon<polygon_t> multipolygon_t;
typedef boost::geometry::model::linestring<point_t> linestring_t;
typedef boost::geometry::model::point<double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree> > sphere_t;

/// \namespace osmobjects
namespace osmobjects {

/// \file osmobjects.hh
/// \brief Data structures for OSM objects
///
/// This file contains class definitions for the 3 objects used for OSM data.
/// These are nodes, which are a single point. These are used to mark a
/// point of interest, like a traffic light,
/// Ways contain multiple points, and are used for buildings and highways.
/// Relations contain multipe ways, and are often used for combining
/// highway segments or administrative boundaries.

typedef enum {none, create, modify, remove} action_t; // delete is a reserved word
typedef enum {empty, node, way, relation, member} osmtype_t;

/// \class OsmObject
/// \brief This is the base class for the common data fields used by all OSM objects
class OsmObject
{
  public:
    /// Add a metadata tag to an OSM object
    void addTag(const std::string &key, const std::string &value) {
        tags[key] = value;
    };

    void setAction(action_t act) { action = act; };
    void setUID(long val) { uid = val; };
    void setChangeID(long val) { change_id = val; };
    
    action_t action = none;     ///< the action that contains this object
    osmtype_t type = empty;     ///< The type of this object, node, way, or relation
    long id = 0;                ///< The object ID within OSM
    int version = 0;            ///< The version of this object
    ptime timestamp;            ///< The timestamp of this object's creation or modification
    long uid = 0;               ///< The User ID of the mapper of this object
    std::string user;           ///< The User name  of the mapper of this object
    long change_id = 0;         ///< The changeset ID this object is contained in
    std::map<std::string, std::string> tags; ///< OSM metadata tags

    bool priority = false;      ///< Whether it's in the priority area
    /// Dump internal data to the terminal, only for debugging
    void dump(void);
    bool containsKey(const std::string &key) {
	return tags.count(key);
    };
    bool containsValue(const std::string &key, const std::string &value) {
	std::string lower = boost::algorithm::to_lower_copy(value);
	if (tags[key].size() == 0 ) {
	    return true;
	}
        for (auto it = tags.begin();  it != tags.end(); ++it) {
            if (it->second == value) {
                return true;
            }
        }
	return false;
    };
};

/// \class OsmNode
/// \brief This represents an OSM node.
///
/// A node has point coordinates, and may contain tags if it's a POI.
class OsmNode: public OsmObject
{
public:
    OsmNode(long nid) { id = nid; };
    point_t point;              ///< The location of this node
    OsmNode(void) { type = node; };
    OsmNode(double lat, double lon) {
        setPoint(lat, lon);
        type = node;
    };
    /// Set the latitude of this node
    void setLatitude(double lat) {
        point.set<0>(lat);
    };
    /// Set the longitude of this node
    void setLongitude(double lon) {
        point.set<1>(lon);
    };
    /// Set the location of this node
    void setPoint(double lat, double lon) {
        point.set<0>(lat);
        point.set<1>(lon);
    };

    /// Dump internal data to the terminal, only for debugging
    void dump(void) {
        OsmObject::dump();
    };
};

/// \class OsmWay
/// \brief This represents an OSM way.
///
/// A way has multiple nodes, and should always have standard OSM tags
/// or it's bad data.
class OsmWay : public OsmObject
{
public:
    OsmWay(long wid) { id = wid; };
    OsmWay(void) { type = way; refs.clear(); };
    
    std::vector<long> refs;     ///< Store all the nodes by reference ID
    linestring_t linestring;    ///< Store the node as a linestring
    polygon_t polygon;          ///< Store the nodes as a polygon
    point_t center;             ///< Store the centroid of the way

    /// Add a reference to a node to this way
    void addRef(long ref) {
        refs.push_back(ref);
    };

    /// Polygons are closed objects, like a building, while a highway
    /// is a linestring
    bool isClosed(void) {
        if (refs.size() > 0) {
            if (refs[0] == refs[refs.size()-1]) {
                return true;
            }
        }
        return false;
    };
    /// Return the number of nodes in this way
    int numPoints(void) { return refs.size(); };

    /// Add a point to the way's geometric data storage
    // void makeLinestring(point_t point);

    /// Calculate the length of the linestring in Kilometers
    double getLength(void) {
        return boost::geometry::length(linestring,
               boost::geometry::strategy::distance::haversine<float>(6371.0));
    };

    /// Dump internal data to the terminal, only for debugging
    void dump(void);
};


/// class OsmRelation
/// \brief This represents an OSM relation.
///
/// A relation contains multiple ways, and contains tags about the relation
class OsmRelation : public OsmObject
{
public:
    OsmRelation(void) { type = relation; };
    
    std::vector<OsmWay> members; ///< The members contained in this relation

    /// Dump internal data to the terminal, only for debugging
    void dump(void);
};

}
// EOF namespace osmobjects

#endif  //  __OSMOBJECTS_HH__    
