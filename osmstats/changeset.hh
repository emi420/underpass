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


/// \brief      This file is for supporting the changetset file format, not
/// to be confused with the OSM Change format. This format is the one used
/// for files from planet, and also supports replication updates. There appear
/// to be no parsers for this format, so this was created to fill that gap.
/// The files are compressed in gzip format, so uncompressing has to be done
/// internally before parsing the XML
///

#ifndef __CHANGESET_HH__
#define __CHANGESET_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <map>
#include <list>
#include <iostream>
// #include <pqxx/pqxx>
#ifdef LIBXML
#  include <libxml++/libxml++.h>
#endif

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "timer.hh"
#include "hotosm.hh"
#include "osmstats/osmstats.hh"
#include "data/geoutil.hh"

// Forward declaration
namespace osmstats {
class RawCountry;
};

// Forward declaration
namespace geoutil {
class GeoUtil;
};

/// \namespace changeset
namespace changeset {

/// \file changeset.hh
/// \brief The file is used for processing changeset files
///
/// The Changeset file contains the raw data on just the change,
/// and doesn't contain any data of the change except the comment
/// and hashtags used when the change was uploaded to OSM.

/// \class ChangeSet
/// \brief Data structure for a single changeset
///
/// This stores the hashtags and comments use for a change when it is
/// uploaded to OSM
class ChangeSet
{
public:
    ChangeSet(void) { /* FIXME */};
#ifdef LIBXML
    ChangeSet(const std::deque<xmlpp::SaxParser::Attribute> attrs);
#endif

    /// Dump internal data to the terminal, used only for debugging
    void dump(void);

    /// Add a hashtag to internal storage
    void addHashtags(const std::string &text) {
        hashtags.push_back(text);
    };

    /// Add the comment field, which is often used for hashtags
    void addComment(const std::string &text) { comment = text; };

    /// Add the editor field
    void addEditor(const std::string &text) { editor = text; };
    
    //osmstats::RawCountry country;
    int countryid;
    // protected so testcases can access private data
// protected:
    // These fields come from the changeset replication file
    long id = 0;                ///< The changeset id
    ptime created_at;           ///< Creation starting timestamp for this changeset
    ptime closed_at;            ///< Creation ending timestamp for this changeset
    bool open = false;          ///< Whether this changeset is still in progress
    std::string user;           ///< The OSM user name making this change
    long uid = 0;               ///< The OSM user ID making this change
    double min_lat = 0.0;       ///< The minimum latitude for the bounding box of this change
    double min_lon = 0.0;       ///< The minimum longitude for the bounding box of this change
    double max_lat = 0.0;       ///< The maximum latitude for the bounding box of this change
    double max_lon = 0.0;       ///< The maximum longitude for the bounding box of this change
    int num_changes = 0;        ///< The number of changes in this changeset, which apears to be unused
    int comments_count = 0;     ///< The number of comments in this changeset, which apears to be unused
    std::vector<std::string> hashtags; ///< Internal aray of hashtags in this changeset
    std::string comment;               ///< The comment for this changeset
    std::string editor;                ///< The OSM editor the end user used
    std::string source;                ///< The imagery source
    std::map<std::string, std::string> tags;
};

/// \class ChangeSetFile
/// \brief This file reads a changeset file
///
/// This class reads a changeset file, as obtained from the OSM
/// planet server. This format is not supported by other tools,
/// so we add it there. As changeset file contains multiple changes,
// this contains data for the entire file.
#ifdef LIBXML
class ChangeSetFile: public xmlpp::SaxParser
#else
class ChangeSetFile
#endif
{
public:
    ChangeSetFile(void) { };

    /// Read a changeset file from disk or memory into internal storage
    bool readChanges(const std::string &file);

    /// Read a changeset file from disk or memory into internal storage
    bool readChanges(const std::vector<unsigned char> &buffer);
    
    /// Import a changeset file from disk and initialize the database
    bool importChanges(const std::string &file);

#ifdef LIBXML
    /// Called by libxml++ for the start of each element in the XML file
    void on_start_element(const Glib::ustring& name,
                          const AttributeList& properties) override;
    /// Called by libxml++ for the end of each element in the XML file
    void on_end_element(const Glib::ustring& name) override;
#endif

    /// Read an istream of the data and parse the XML
    bool readXML(std::istream & xml);

    /// Setup the boundary data used to determine the country
    bool setupBoundaries(std::shared_ptr<geoutil::GeoUtil> &geou) {
        // boundaries = geou;
        return false;
    };

    /// Get one set of change data from the parsed XML data
    ChangeSet& operator[](int index) { return changes[index]; };
    
    /// Dump the data of this class to the terminal. This should only
    /// be used for debugging.
    void dump(void);
// protected:
//     bool store;
    std::string filename;       ///< The filename of this changeset for disk files
    std::vector<ChangeSet> changes; ///< Storage of all the changes in this data
    // std::shared_ptr<geoutil::GeoUtil> boundaries; ///< A pointer to the geoboundary data
};
}       // EOF changeset

#endif  // EOF __CHANGESET_HH__
