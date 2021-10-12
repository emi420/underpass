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

#ifndef __OSM2PGSQL_HH__
#define __OSM2PGSQL_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <regex>
#include <pqxx/nontransaction>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "pq.hh"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "osmstats/osmchange.hh"
using namespace osmchange;

/// \file osm2pgsql.hh
/// \brief Manages a osm2pgsql DB

/// \namespace osm2pgsql
namespace osm2pgsql {

///
/// \brief The Osm2Pgsql class handles the communication with an osm2pgsql DB.
///
/// Methods to query the osm2pgsql DB status and to update the DB with osm changes
/// are provided.
///
class Osm2Pgsql : public pq::Pq {
  public:
    ///
    /// \brief OSM2PGSQL_DEFAULT_SCHEMA_NAME the default schema name for osm2pgsql tables.
    ///
    static const std::string OSM2PGSQL_DEFAULT_SCHEMA_NAME;

    ///
    /// \brief Osm2Pgsql constructs an Osm2Pgsql from arguments.
    /// \param dburl the DB url in the form USER:PASSSWORD@HOST/DATABASENAME
    /// \param schema name of the osm2pgsql schema, defaults to "public".
    ///
    Osm2Pgsql(const std::string &dburl, const std::string &schema = OSM2PGSQL_DEFAULT_SCHEMA_NAME);

    ///
    /// \brief Osm2Pgsql constructs a default uninitialized Osm2Pgsql.
    ///        Call connect() and setSchema() to initialize.
    ///
    Osm2Pgsql() = default;

    ///
    /// \brief getLastUpdate
    /// \return the last timestamp in the DB
    ///
    ptime getLastUpdate();

#if 0

    // osm2pgsql fork is disabled but left in the sources because it may be useful to
    // compare the results of the original application with those of our clone
    // implementation.

    ///
    /// \brief updateDatabase updates the DB with osm changes.
    /// \param osm_changes input data (decompressed) from an OSC file.
    /// \return TRUE on success, errors are logged.
    ///
    bool updateDatabase(const std::string &osm_changes);

#endif

    ///
    /// \brief updateDatabase updates the DB with osm changes.
    /// \param osm_changes OsmChangeFile (parsed)
    /// \return TRUE on success, errors are logged.
    ///
    bool updateDatabase(const std::shared_ptr<OsmChangeFile> &osm_changes) const;

    ///
    /// \brief upsertWay inserts or updates a way in the osm2pgsql DB.
    /// \param way the way that will be inserted or updated.
    /// \return TRUE on success, errors are logged.
    ///
    bool upsertWay(const std::shared_ptr<osmobjects::OsmWay> &way) const;

    ///
    /// \brief upsertNode inserts or updates a node in the osm2pgsql DB.
    /// \param node the node that will be inserted or updated.
    /// \return TRUE on success, errors are logged.
    ///
    bool upsertNode(const std::shared_ptr<osmobjects::OsmNode> &node) const;

    ///
    /// \brief upsertRelation inserts or updates a relation in the osm2pgsql DB.
    /// \param relation the relation that will be inserted or updated.
    /// \return TRUE on success, errors are logged.
    ///
    bool upsertRelation(const std::shared_ptr<osmobjects::OsmRelation> &relation) const;

    ///
    /// \brief removeWay removes a way from the osm2pgsql DB.
    /// \param way to be removed.
    /// \return TRUE on success, errors are logged.
    ///
    bool removeWay(const std::shared_ptr<osmobjects::OsmWay> &way) const;

    ///
    /// \brief removeNode removes a node from the osm2pgsql DB.
    /// \param node to be removed.
    /// \return TRUE on success, errors are logged.
    ///
    bool removeNode(const std::shared_ptr<osmobjects::OsmNode> &node) const;

    ///
    /// \brief removeRelation removes a relation from the osm2pgsql DB.
    /// \param relation to be removed.
    /// \return TRUE on success, errors are logged.
    ///
    bool removeRelation(const std::shared_ptr<osmobjects::OsmRelation> &relation) const;

    ///
    /// \brief connects to the DB using \a dburl
    /// \param dburl the connection string to the DB.
    /// \return TRUE on success.
    ///
    bool connect(const std::string &dburl);

    ///
    /// \brief getSchema returns the schema name for osm2pgsql tables.
    /// \return the schema name.
    ///
    const std::string &getSchema() const;

    ///
    /// \brief setSchema sets the the schema name for osm2pgsql tables.
    /// \param newSchema the schema name.
    ///
    void setSchema(const std::string &newSchema);

  private:
    /// Get last timestamp in the DB
    bool getLastUpdateFromDb();

    struct Polygon {
        Polygon() = default;
        Polygon(long outer_ring)
        {
            outer.push_back(outer_ring);
        };
        std::list<long> outer;
        std::string inner;
        long id = std::numeric_limits<long>::lowest();
    };

    struct TagParser {

        std::string tag_field_names;
        std::string tag_field_values;
        std::string tag_field_updates;
        std::string tags_hstore_literal{"E'"};
        std::string tags_array_literal{"E'{"};

        bool is_road = false;
        bool is_polygon = false;
        bool has_generic_key = false;
        int z_order = 0;

        static const std::regex tags_escape_re;
        static constexpr auto separator{", "};

        /// These tags are stored in columns
        static const std::set<std::string> column_stored_tags;

        /// These additional tags are stored in columns for points
        static const std::set<std::string> column_points_stored_tags;

        /// These tags make a polygon
        static const std::set<std::string> polygon_tags;

        /// Objects without any of the following keys will be deleted
        static const std::set<std::string> generic_keys;

        /// Array used to specify z_order per key/value combination.
        /// Each element has the form {key, value}, {z_order, is_road}.
        /// If is_road=1, the object will be added to planet_osm_roads.
        static const std::map<std::pair<std::string, std::string>, std::pair<bool, int>> z_index_map;

        void parse(const std::map<std::string, std::string> &tags, const pqxx::nontransaction &worker, bool is_point);
    };

    // Returns a list of not closed ways and their being/end nodes
    std::map<long, std::pair<long, long>> notClosedWays(const std::list<long> line_ids, pqxx::nontransaction &worker) const;

    ptime last_update = not_a_date_time;
    std::string dburl;

    /// Default schema name for osm2pgsql ("public"), for simplicity, we are using
    /// the same schema for data and "middle" tables.
    std::string schema = OSM2PGSQL_DEFAULT_SCHEMA_NAME;
};

} // namespace osm2pgsql

#endif
