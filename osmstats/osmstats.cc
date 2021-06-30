//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <pqxx/pqxx>
#include "pqxx/nontransaction"

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_facet.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

using namespace boost::math;
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "osmstats/osmstats.hh"
#include "osmstats/changeset.hh"
#include "data/osmobjects.hh"
#include "data/underpass.hh"

using namespace apidb;

/// \namespace osmstats
namespace osmstats {

QueryOSMStats::QueryOSMStats(void)
{
}

QueryOSMStats::QueryOSMStats(const std::string &dbname)
{
    if (dbname.empty()) {
        // Validate environment variable is defined.
        char *tmp = std::getenv("STATS_DB_URL");
        db_url = tmp;
    } else {
        db_url = "dbname = " + dbname;
    }
    connect(dbname);
};

bool
QueryOSMStats::connect(void)
{
    return connect("osmstats");
}

bool
QueryOSMStats::connect(const std::string &dbname)
{
    if (dbname.empty()) {
	std::cerr << "ERROR: need to specify database name!" << std::endl;
    }
    
    try {
        std::string args = "dbname = " + dbname;
	sdb = std::make_shared<pqxx::connection>(args);
	if (sdb->is_open()) {
            std::cout << "Opened database connection to " << dbname  << std::endl;
	    return true;
	} else {
	    return false;
	}
    } catch (const std::exception &e) {
	std::cerr << "ERROR: Couldn't open database connection to " << dbname  << std::endl;
	std::cerr << e.what() << std::endl;
	return false;
   }    
}

// long
// QueryOSMStats::updateCounters(std::map<const std::string &, long> data)
// {
//     for (auto it = std::begin(data); it != std::end(data); ++it) {
//     }
// }

int
QueryOSMStats::lookupHashtag(const std::string &hashtag)
{
    std::string query = "SELECT id FROM taw_hashtags WHERE hashtag=\'";
    query += hashtag + "\';";
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    // There is only one value returned
    return result[0][0].as(int(0));
}

bool
QueryOSMStats::applyChange(osmchange::ChangeStats &change)
{
    std::cout << "Applying OsmChange data" << std::endl;

    if (hasHashtag(change.change_id)) {
        std::cout << "Has hashtag for id: " << change.change_id << std::endl;
    } else {
        std::cerr << "No hashtag for id: " << change.change_id << std::endl;
    }

    std::string ahstore;
    if (change.added.size() > 0) {
	ahstore = "HSTORE(ARRAY[";
        for (auto it = std::begin(change.added); it != std::end(change.added); ++it) {
	    if (it->first.empty()) {
		return true;
	    }
            if (it->second > 0) {
                ahstore += " ARRAY[\'" + it->first + "\',\'" + std::to_string((int)it->second) +"\'],";
            }
        }
        ahstore.erase(ahstore.size() - 1);
        ahstore += "])";
    } else {
        ahstore.clear();
    }
    if (change.modified.size() > 0) {
	ahstore = "HSTORE(ARRAY[";
        for (auto it = std::begin(change.modified); it != std::end(change.modified); ++it) {
	    if (it->first.empty()) {
		return true;
	    }
            if (it->second > 0) {
                ahstore += " ARRAY[\'" + it->first + "\',\'" + std::to_string(it->second) +"\'],";
            }
        }
        ahstore.erase(ahstore.size() - 1);
        ahstore += "])";
    }

    // Some of the data field in the changset come from a different file,
    // which may not be downloaded yet.
    ptime now = boost::posix_time::microsec_clock::local_time();
    std::string aquery;
    if (ahstore.size() > 0) {
	if (change.added.size() > 0) {
	    aquery = "INSERT INTO changesets (id, user_id, updated_at, added)";
	} else if (change.modified.size() > 0) {
	    aquery = "INSERT INTO changesets (id, user_id, updated_at, modified)";
	}
    } else {
	aquery = "INSERT INTO changesets (id, user_id, updated_at)";
    }
    aquery += " VALUES(" + std::to_string(change.change_id) + ", ";
    aquery += std::to_string(change.user_id) + ", ";
    aquery += "\'" + to_simple_string(now) + "\', ";
    if (ahstore.size() > 0) {
	if (change.added.size() > 0) {
	    aquery += ahstore + ") ON CONFLICT (id) DO UPDATE SET added = " + ahstore + ",";
	} else {
	    aquery += ahstore + ") ON CONFLICT (id) DO UPDATE SET modified = " + ahstore + ",";
	}
    } else {
	aquery.erase(aquery.size() - 2);
	aquery += ") ON CONFLICT (id) DO UPDATE SET";
    }
    
    aquery += " updated_at = \'" + to_simple_string(now) + "\'";
    aquery += " WHERE changesets.id=" + std::to_string(change.change_id);

    std::cout << "QUERY stats: " << aquery << std::endl;
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(aquery);

    worker.commit();
}

bool
QueryOSMStats::applyChange(changeset::ChangeSet &change)
{
    std::cout << "Applying ChangeSet data" << std::endl;
    change.dump();

    // Some old changefiles have no user information
    std::string query = "INSERT INTO users VALUES(";
    query += std::to_string(change.uid) + ",\'" + sdb->esc(change.user);
    query += "\') ON CONFLICT DO NOTHING;";
    std::cout << "QUERY: " << query << std::endl;
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    //worker.commit();

    // If there are no hashtags in this changset, then it isn't part
    // of an organized map campaign, so we don't need to store those
    // statistics except for editor usage.
#if 0
    underpass::Underpass under;
    under.connect();
    if (change.hashtags.size() == 0) {
        std::cout << "No hashtags in change id: " << change.id << std::endl;
        under.updateCreator(change.uid, change.id, change.editor);
        worker.commit();
        // return true;
    } else {
        std::cout << "Found hashtags for change id: " << change.id << std::endl;
    }
#endif
    // Add changeset data
    // road_km_added | road_km_modified | waterway_km_added | waterway_km_modified | roads_added | roads_modified | waterways_added | waterways_modified | buildings_added | buildings_modified | pois_added | pois_modified | updated_at
    // the updated_at timestamp is set after the change data has been processed

    // osmstats=# UPDATE raw_changesets SET road_km_added = (SELECT road_km_added + 10.0 FROM raw_changesets WHERE road_km_added>0 AND user_id=649260 LIMIT 1) WHERE user_id=649260;

    query = "INSERT INTO changesets (id, editor, user_id, created_at";
    // if (change.open) {
    //     query = "INSERT INTO changesets (id, editor, user_id, created_at";
    // } else {
    //     query = "INSERT INTO changesets (id, editor, user_id, created_at, closed_at";
    // }
    if (change.hashtags.size() > 0) {
        query += ", hashtags ";        
    }
    if (!change.source.empty()) {
        query += ", source ";
    }
    query += ", bbox) VALUES(";
    query += std::to_string(change.id) + ",\'" + change.editor + "\',\'";\
    query += std::to_string(change.uid) + "\',\'";
    query += to_simple_string(change.created_at) + "\'";
    // if (!change.open) {
    //     query += ",\'" + to_simple_string(change.closed_at) + "\'";
    // }
    // Hashtags are only used in mapping campaigns using Tasking Manager
    if (change.hashtags.size() > 0) {
        query += ", \'{ ";
        for (auto it = std::begin(change.hashtags); it != std::end(change.hashtags); ++it) {
            boost::algorithm::replace_all(*it, "\"", "&quot;");
            query += "\"" + *it + "\"" + ", ";
        }
        // drop the last comma
        query.erase(query.size() - 2);
        query += " }\'";
    }
    // The source field is not always present
    if (!change.source.empty()) {
        query += ",\'" + change.source += "\'";
    }

    // Store the current values as they may get changed to expand very short
    // lines or POIs so they have a bounding box big enough for postgis to use.
    double min_lat = change.min_lat;
    double max_lat = change.max_lat;
    double min_lon = change.min_lon;
    double max_lon = change.max_lon;
    // FIXME: There are bugs in the bounding box coordinates in some of the older
    // changeset files, but for now ignore these.
    double fff = relative_difference(max_lon, min_lon);
    //std::cout << "FIXME: Float diff " << fff << std::endl;
    int fudge = 0.0001;
    if (fff < fudge) {
        std::cout << "FIXME: line too short! " << fff << std::endl;
        return false;
    }
    // a changeset with a single node in it doesn't draw a line
    if (change.max_lon < 0 && change.min_lat < 0) {
        std::cerr << "WARNING: single point! " << change.id << std::endl;
	min_lat = change.min_lat + (fudge/2);
	max_lat = change.max_lat + (fudge/2);
	min_lon = change.min_lon - (fudge/2);
	max_lon = change.max_lon - (fudge/2);
        //return false;
    }
    if (max_lon == min_lon || max_lat == min_lat) {
        std::cerr << "WARNING: not a line! " << change.id << std::endl;
	min_lat = change.min_lat + (fudge/2);
	max_lat = change.max_lat + (fudge/2);
	min_lon = change.min_lon - (fudge/2);
	max_lon = change.max_lon - (fudge/2);
	// return false;
    }
    if (max_lon < 0 && min_lat < 0) {
        std::cerr << "WARNING: single point! " << change.id << std::endl;
	min_lat = change.min_lat + (fudge/2);
	max_lat = change.max_lat + (fudge/2);
	min_lon = change.min_lon - (fudge/2);
	max_lon = change.max_lon - (fudge/2);
        // return false;
    }
    if (change.num_changes == 0) {
        std::cerr << "WARNING: no changes! " << change.id << std::endl;
        return false;
    }
    // Add the bounding box of the changeset here next
        // long_high,lat_high,
	// long_low,lat_high,
	// long_low,lat_low,
	// long_high,lat_low,
	// long_high,lat_high
    std::string bbox;
    bbox += ", ST_MULTI(ST_GeomFromEWKT(\'SRID=4326;POLYGON((";
    // Upper left
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(max_lat) + ",";
    // Upper right
    bbox += std::to_string(min_lon) + "  ";
    bbox += std::to_string(max_lat) + ",";
    // Lower right
    bbox += std::to_string(min_lon) + "  ";
    bbox += std::to_string(min_lat) + ",";
    // Lower left
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(min_lat) + ",";
    // Close the polygon
    bbox += std::to_string(max_lon) + "  ";
    bbox += std::to_string(max_lat) + ")";

    query += bbox;

    query += ")\')";
    //query += ")) ON CONFLICT DO NOTHING;";
    query += ")) ON CONFLICT (id) DO UPDATE SET editor=\'" + change.editor;
    query += "\', created_at=\'" + to_simple_string(change.created_at);
    // if (!change.open) {
    // 	query += "\', closed_at=\'" + to_simple_string(change.closed_at);
    // }
    query += "\', bbox=" + bbox.substr(2) + ")'))";
    std::cout << "QUERY: " << query << std::endl;
    result = worker.exec(query);

    // Commit the results to the database
    worker.commit();

    return true;
}

// Populate internal storage of a few heavily used data, namely
// the indexes for each user, country, or hashtag.
bool
QueryOSMStats::populate(void)
{
    // Get the country ID from the raw_countries table
    std::string query = "SELECT id,name,code FROM raw_countries;";
    pqxx::work worker(*db);
    pqxx::result result = worker.exec(query);
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawCountry rc(it);
        // addCountry(rc);
        // long id = std::stol(it[0].c_str());
        countries.push_back(rc);
    };

    query = "SELECT id,name FROM users;";
    pqxx::work worker2(*db);
    result = worker2.exec(query);
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawUser ru(it);
        users.push_back(ru);
    };

#if 0
    query = "SELECT id,hashtag FROM raw_hashtags;";
    result = worker.exec(query);
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawHashtag rh(it);
        hashtags[rh.name] = rh;
    };
#endif
    // ptime start = time_from_string("2010-07-08 13:29:46");
    // ptime end = second_clock::local_time();
    // long roadsAdded = QueryStats::getCount(QueryStats::highway, 0,
    //                                        QueryStats::totals, start, end);
    // long roadKMAdded = QueryStats::getLength(QueryStats::highway, 0,
    //                                          start, end);
    // long waterwaysAdded = QueryStats::getCount(QueryStats::waterway, 0,
    //                                            QueryStats::totals, start, end);
    // long waterwaysKMAdded = QueryStats::getLength(QueryStats::waterway, 0,
    //                                               start, end);
    // long buildingsAdded = QueryStats::getCount(QueryStats::waterway, 0,
    //                                            QueryStats::totals, start, end);

    worker2.commit();

    return true;
};

bool
QueryOSMStats::getRawChangeSets(std::vector<long> &changeset_ids)
{
    pqxx::work worker(*db);
    std::string sql = "SELECT id,road_km_added,road_km_modified,waterway_km_added,waterway_km_modified,roads_added,roads_modified,waterways_added,waterways_modified,buildings_added,buildings_modified,pois_added,pois_modified,editor,user_id,created_at,closed_at,verified,augmented_diffs,updated_at FROM changesets WHERE id=ANY(ARRAY[";
    // Build an array string of the IDs
    for (auto it = std::begin(changeset_ids); it != std::end(changeset_ids); ++it) {
        sql += std::to_string(*it);
        if (*it != changeset_ids.back()) {
            sql += ",";
        }
    }
    sql += "]);";

    std::cout << "QUERY: " << sql << std::endl;
    pqxx::result result = worker.exec(sql);
    std::cout << "SIZE: " << result.size() <<std::endl;
    //OsmStats stats(result);
    worker.commit();

    for (auto it = std::begin(result); it != std::end(result); ++it) {
        RawChangeset os(it);
        ostats.push_back(os);
    }

    return true;
}

bool
QueryOSMStats::hasHashtag(long changeid)
{
#if 0
    std::string query = "SELECT COUNT(hashtag_id) FROM changesets_hashtags WHERE changeset_id=" + std::to_string(changeid) + ";";
    std::cout << "QUERY: " << query << std::endl;
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    if (result[0][0].as(int(0)) > 0 ) {
        return true;
    }
#endif
    return false;
}

// Get the timestamp of the last update in the database
ptime
QueryOSMStats::getLastUpdate(void)
{
    std::string query = "SELECT MAX(created_at) FROM changesets;";
    std::cout << "QUERY: " << query << std::endl;
    // auto worker = std::make_shared<pqxx::work>(*db);
    pqxx::work worker(*sdb);
    pqxx::result result = worker.exec(query);
    worker.commit();

    ptime last;
    if (result[0][0].size() > 0) {
        last = time_from_string(result[0][0].c_str());
        return last;
    }
    return not_a_date_time;
}

void
QueryOSMStats::dump(void)
{
    for (auto it = std::begin(ostats); it != std::end(ostats); ++it) {
        it->dump();
    }
}

// Write the list of hashtags to the database
int
QueryOSMStats::updateRawHashtags(void)
{
//    INSERT INTO keys(name, value) SELECT 'blah', 'true' WHERE NOT EXISTS (SELECT 1 FROM keys WHERE name='blah');

#if 0
    std::string query = "INSERT INTO raw_hashtags(hashtag) VALUES(";
    // query += "nextval('raw_hashtags_id_seq'), \'" + tag;
    // query += "\'" + tag;
    query += "\') ON CONFLICT DO NOTHING";
    std::cout << "QUERY: " << query << std::endl;
    pqxx::work worker(*db);
    pqxx::result result = worker.exec(query);
    worker.commit();
    return result.size();
#endif
}

RawChangeset::RawChangeset(pqxx::const_result_iterator &res)
{
    id = std::stol(res[0].c_str());
    for (auto it = std::begin(res); it != std::end(res); ++it) {
        if (it->type() == 20 || it->type() == 23) {
            if (it->name() != "id" && it->name() != "user_id") {
                counters[it->name()] = it->as(long(0));
            }
            // FIXME: why are there doubles in the schema at all ? It's
            // a counter, so should always be an integer or long
        } else if (it->type() == 701) { // double
            counters[it->name()] = it->as(double(0));
        }
    }
    editor = pqxx::to_string(res[13]);
    user_id = std::stol(res[14].c_str());
    created_at = time_from_string(pqxx::to_string(res[15]));
    closed_at = time_from_string(pqxx::to_string(res[16]));
    // verified = res[17].bool();
    // augmented_diffs = res[18].num();
    updated_at = time_from_string(pqxx::to_string(res[19]));
}

void
RawChangeset::dump(void)
{
    std::cout << "-----------------------------------" << std::endl;
    std::cout << "changeset id: \t\t " << id << std::endl;
    std::cout << "Editor: \t\t " << editor << std::endl;
    std::cout << "User ID: \t\t "  << user_id << std::endl;
    std::cout << "Created At: \t\t " << created_at << std::endl;
    std::cout << "Closed At: \t\t " << closed_at << std::endl;
    std::cout << "Verified: \t\t " << verified << std::endl;
    // std::cout << augmented_diffs << std::endl;
    std::cout << "Updated At: \t\t " << updated_at << std::endl;
}

}       // EOF osmstatsdb

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
