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

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include <array>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <pqxx/pqxx>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>
#ifdef LIBXML
#include <libxml++/libxml++.h>
#endif

#include <glibmm/convert.h>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/visitor.hpp>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/filesystem.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/tokenizer.hpp>
#include <boost/tokenizer.hpp>
#include <boost/timer/timer.hpp>

#include "galaxy/changeset.hh"
#include "galaxy/galaxy.hh"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

#include "log.hh"
using namespace logger;

/// \namespace changeset
namespace changeset {

/// Check a character in a string if it's a control character
bool
IsControl(int i)
{
    return (iscntrl(i));
}

/// Read a changeset file from disk, which may be a huge file
/// Since it is a huge file, process in pieces and don't store
/// anything except in the database. A changeset file entry
/// looks like this:
///
/// <changeset id="12345" created_at="2014-10-10T01:57:09Z"
/// closed_at="2014-10-10T01:57:23Z" open="false" user="foo" uid="54321"
/// min_lat="-2.8042325" min_lon="29.5842812" max_lat="-2.7699398"
/// max_lon="29.6012844" num_changes="569" comments_count="0">
///  <tag k="source" v="Bing"/>
///  <tag k="comment" v="#hotosm-task-001 #redcross #missingmaps"/>
///  <tag k="created_by" v="JOSM/1.5 (7182 en)"/>
/// </changeset>
bool
ChangeSetFile::importChanges(const std::string &file)
{
    std::ifstream change;
    int size = 0;
    //    store = false;

#ifdef LIBXML
    // FIXME: this should really use CHUNKS, since the files can
    // many gigs.
    try {
        set_substitute_entities(true);
        parse_file(file);
    } catch (const xmlpp::exception &ex) {
        // FIXME: files downloaded seem to be missing a trailing \n,
        // so produce an error, but we can ignore this as the file is
        // processed correctly.
        // log_error(_("libxml++ exception: %1%"), ex.what());
        int return_code = EXIT_FAILURE;
    }
#endif

    galaxy::QueryGalaxy ostats;
    ostats.connect("galaxy");
    for (auto it = std::begin(changes); it != std::end(changes); ++it) {
        // ostats.applyChange(*it);
    }

    change.close();
    // FIXME: return real value
    return false;
}

bool
ChangeSetFile::readChanges(const std::vector<unsigned char> &buffer)
{

    // parse_memory((const Glib::ustring &)buffer);
}

// Read a changeset file from disk or memory into internal storage
bool
ChangeSetFile::readChanges(const std::string &file)
{
    std::ifstream change;
    int size = 0;
    //    store = false;

    unsigned char *buffer;
    log_debug(_("Reading changeset file %1% "), file);
    std::string suffix = boost::filesystem::extension(file);
    // It's a gzipped file, common for files downloaded from planet
    std::ifstream ifile(file, std::ios_base::in | std::ios_base::binary);
    if (suffix == ".gz") { // it's a compressed file
                           //    if (file[0] == 0x1f) {
        change.open(file, std::ifstream::in | std::ifstream::binary);
        try {
            boost::iostreams::filtering_streambuf<boost::iostreams::input>
                inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            inbuf.push(ifile);
            std::istream instream(&inbuf);
            // log_debug(_(instream.rdbuf();
            readXML(instream);
        } catch (std::exception &e) {
            log_error(_("opening %1% %2%"), file, e.what());
            // return false;
        }
    } else { // it's a text file
        change.open(file, std::ifstream::in);
        readXML(change);
    }

#if 0
    // magic number: 0x8b1f or 0x1f8b for gzipped
    // <?xml for text
    std::string foo = "Hello World";
    boost::iostreams::array_source(foo.c_str(), foo.size());
    // boost::iostreams::filtering_streambuf<boost::iostreams::input> fooby(foo, 10);
#endif
    change.close();
}

void
ChangeSetFile::areaFilter(const multipolygon_t &poly)
{
#if TIMING_DEBUG_X
    boost::timer::auto_cpu_timer timer("ChangeSetFile::areaFilter: took %w seconds\n");
#endif
    // log_debug(_("Pre filtering changeset size is %1%"), changes.size());
    for (auto it = std::begin(changes); it != std::end(changes); it++) {
        ChangeSet *change = it->get();
        point_t pt;
        boost::geometry::append(change->bbox, point_t(change->max_lon, change->max_lat));
        boost::geometry::append(change->bbox, point_t(change->max_lon, change->min_lat));
        boost::geometry::append(change->bbox, point_t(change->min_lon, change->min_lat));
        boost::geometry::append(change->bbox, point_t(change->min_lon, change->max_lat));
        boost::geometry::append(change->bbox, point_t(change->max_lon, change->max_lat));
        boost::geometry::centroid(change->bbox, pt);
        if (poly.empty()) {
            // log_debug(_("Accepting changeset %1% as in priority area because area information is missing"),
            // change->id);
            change->priority = true;
        } else {
            if (!boost::geometry::within(pt, poly)) {
                // log_debug(_("Validating changeset %1% is not in a priority
                // area"), change->id);
                change->priority = false;
                changes.erase(it--);
            } else {
                // log_debug(_("Validating changeset %1% is in a priority area"),
                // change->id);
                change->priority = true;
            }
        }
    }
    // log_debug(_("Post filtering changeset size is %1%"),
    // changeset->changes.size());
}

void
ChangeSet::dump(void)
{
    std::cerr << "-------------------------" << std::endl;
    std::cerr << "Change ID: " << id << std::endl;
    std::cerr << "Created At:  " << to_simple_string(created_at) << std::endl;
    std::cerr << "Closed At:   " << to_simple_string(closed_at) << std::endl;
    if (open) {
        std::cerr << "Open change: true" << std::endl;
    } else {
        std::cerr << "Open change: false" << std::endl;
    }
    std::cerr << "User:        " << user << std::endl;
    std::cerr << "User ID:     " << uid << std::endl;
    std::cerr << "Min Lat:     " << min_lat << std::endl;
    std::cerr << "Min Lon:     " << min_lon << std::endl;
    std::cerr << "Max Lat:     " << max_lat << std::endl;
    std::cerr << "Max Lon:     " << max_lon << std::endl;
    std::cerr << "Changes:     " << num_changes << std::endl;
    if (!source.empty()) {
        std::cerr << "Source:      " << source << std::endl;
    }
    // std::cerr << "Comments:    " << comments_count << std::endl;
    for (auto it = std::begin(hashtags); it != std::end(hashtags); ++it) {
        std::cerr << "Hashtags:    " << *it << std::endl;
    }
    if (!comment.empty()) {
        std::cerr << "Comments:    " << comment << std::endl;
    }
    std::cerr << "Editor:      " << editor << std::endl;
}

#ifdef LIBXML
ChangeSet::ChangeSet(const std::deque<xmlpp::SaxParser::Attribute> attributes)
{
    // On non-english numeric locales using decimal separator different than '.'
    // this is necessary to parse double strings with std::stod correctly without
    // loosing precision
    std::setlocale(LC_NUMERIC, "C");

    for (const auto &attr_pair: attributes) {
        try {
            if (attr_pair.name == "id") {
                id = std::stol(attr_pair.value); // change id
            } else if (attr_pair.name == "created_at") {
                created_at =
                    from_iso_extended_string(attr_pair.value.substr(0, 19));
            } else if (attr_pair.name == "closed_at") {
                closed_at =
                    from_iso_extended_string(attr_pair.value.substr(0, 19));
            } else if (attr_pair.name == "open") {
                if (attr_pair.value == "true") {
                    open = true;
                } else {
                    open = false;
                }
            } else if (attr_pair.name == "user") {
                user = attr_pair.value;
            } else if (attr_pair.name == "source") {
                source = attr_pair.value;
            } else if (attr_pair.name == "uid") {
                uid = std::stol(attr_pair.value);
            } else if (attr_pair.name == "lat") {
                min_lat = std::stod(attr_pair.value);
                max_lat = std::stod(attr_pair.value);
            } else if (attr_pair.name == "min_lat") {
                min_lat = std::stod(attr_pair.value);
            } else if (attr_pair.name == "max_lat") {
                max_lat = std::stod(attr_pair.value);
            } else if (attr_pair.name == "lon") {
                min_lon = std::stod(attr_pair.value);
                max_lon = std::stod(attr_pair.value);
            } else if (attr_pair.name == "min_lon") {
                min_lon = std::stod(attr_pair.value);
            } else if (attr_pair.name == "max_lon") {
                max_lon = std::stod(attr_pair.value);
            } else if (attr_pair.name == "num_changes") {
                num_changes = std::stoi(attr_pair.value);
            } else if (attr_pair.name == "changes_count") {
                num_changes = std::stoi(attr_pair.value);
            } else if (attr_pair.name == "comments_count") {
            }
        } catch (const Glib::ConvertError &ex) {
            log_error(_("ChangeSet::ChangeSet(): Exception caught while "
                        "converting values for std::cout: "),
                      ex.what());
        }
    }
}
#endif // EOF LIBXML

void
ChangeSetFile::dump(void)
{
    std::cerr << "There are " << changes.size() << " changes" << std::endl;
    for (auto it = std::begin(changes); it != std::end(changes); ++it) {
        // it->dump();
    }
}

// Read an istream of the data and parse the XML
//
bool
ChangeSetFile::readXML(std::istream &xml)
{
#ifdef LIBXML
    // libxml calls on_element_start for each node, using a SAX parser,
    // and works well for large files.
    try {
        set_substitute_entities(true);
        parse_stream(xml);
    } catch (const xmlpp::exception &ex) {
        // FIXME: files downloaded seem to be missing a trailing \n,
        // so produce an error, but we can ignore this as the file is
        // processed correctly.
        log_error(_("libxml++ exception: %1%"), ex.what());
        return false;
    }
#else
    // Boost::parser_tree with RapidXML is faster, but builds a DOM tree
    // so loads the entire file into memory. Most replication files for
    // hourly or minutely changes are small, so this is better for that
    // case.
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(xml, pt);

    if (pt.empty()) {
        log_error(_("ERROR: XML data is empty!"));
        return false;
    }

    for (auto value: pt.get_child("osm")) {
        if (value.first == "changeset") {
            changeset::ChangeSet change;
            // Process the tags. These don't exist for every element
            for (auto tag: value.second) {
                if (tag.first == "tag") {
                    std::string key = tag.second.get("<xmlattr>.k", "");
                    std::string val = tag.second.get("<xmlattr>.v", "");
                    change.tags[key] = val;
                }
            }
            // Process the attributes, which do exist in every element
            change.id = value.second.get("<xmlattr>.id", 0);
            change.created_at =
                value.second.get("<xmlattr>.created_at",
                                 boost::posix_time::second_clock::local_time());
            change.closed_at =
                value.second.get("<xmlattr>.closed_at",
                                 boost::posix_time::second_clock::local_time());
            change.open = value.second.get("<xmlattr>.open", false);
            change.user = value.second.get("<xmlattr>.user", "");
            change.uid = value.second.get("<xmlattr>.uid", 0);
            change.min_lat = value.second.get("<xmlattr>.min_lat", 0.0);
            change.min_lon = value.second.get("<xmlattr>.min_lon", 0.0);
            change.max_lat = value.second.get("<xmlattr>.max_lat", 0.0);
            change.max_lon = value.second.get("<xmlattr>.max_lon", 0.0);
            change.num_changes = value.second.get("<xmlattr>.num_changes", 0);
            change.comments_count =
                value.second.get("<xmlattr>.comments_count", 0);
            changes.push_back(change);
        }
    }
#endif
    return true;
}

#ifdef LIBXML
void
ChangeSetFile::on_end_element(const Glib::ustring &name)
{
    // log_debug(_("Element \'%1%\' ending"), name);
}

void
ChangeSetFile::on_start_element(const Glib::ustring &name,
                                const AttributeList &attributes)
{
    // log_debug(_("Element \'" << name << "\' starting" << std::endl;
    if (name == "changeset") {
        auto change = std::make_shared<changeset::ChangeSet>(attributes);
        changes.push_back(change);
        if (change->closed_at != not_a_date_time && (last_closed_at == not_a_date_time || change->closed_at > last_closed_at)) {
            last_closed_at = change->closed_at;
        }
        // changes.back().dump();
    } else if (name == "tag") {
        // We ignore most of the attributes, as they're not used for OSM stats.
        // Processing a tag requires multiple passes through the loop. The
        // tho tags to look for are 'k' (keyword) and 'v' (value). So when
        // we see a key we want, we have to wait for the next iteration of
        // the loop to get the value.
        bool hashit = false;
        bool comhit = false;
        bool cbyhit = false;
        bool min_lathit = false;
        bool min_lonhit = false;
        bool max_lathit = false;
        bool max_lonhit = false;
        double min_lat = 0.0;
        double min_lon = 0.0;
        double max_lat = 0.0;
        double max_lon = 0.0;

        for (const auto &attr_pair: attributes) {
            // std::wcout << "\tPAIR: " << attr_pair.name << " = " <<
            // attr_pair.value << std::endl;
            if (attr_pair.name == "k" && attr_pair.value == "max_lat") {
                max_lat = std::stod(attr_pair.value);
                max_lathit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "min_lat") {
                min_lathit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "lat") {
                min_lathit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "max_lon") {
                max_lonhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "min_lon") {
                min_lonhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "lon") {
                min_lonhit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "hashtags") {
                hashit = true;
            } else if (attr_pair.name == "k" && attr_pair.value == "comment") {
                comhit = true;
            } else if (attr_pair.name == "k" &&
                       attr_pair.value == "created_by") {
                cbyhit = true;
            }

            if (hashit && attr_pair.name == "v") {
                hashit = false;
                std::size_t pos = attr_pair.value.find('#', 0);
                if (pos != std::string::npos) {
                    // Don't allow really short hashtags, they're usually a typo
                    if (attr_pair.value.length() < 3) {
                        continue;
                    }
                    char *token =
                        std::strtok((char *)attr_pair.value.c_str(), "#;");
                    while (token != NULL) {
                        token = std::strtok(NULL, "#;");
                        if (token) {
                            changes.back()->addHashtags(token);
                        }
                    }
                } else {
                    changes.back()->addHashtags(attr_pair.value);
                }
            }
            // Hashtags start with an # of course. The hashtag tag wasn't
            // added till later, so many older hashtags are in the comment
            // field instead.
            if (comhit && attr_pair.name == "v") {
                comhit = false;
                changes.back()->addComment(attr_pair.value);
                if (attr_pair.value.find('#') != std::string::npos) {
                    std::vector<std::string> result;
                    boost::split(result, attr_pair.value,
                                 boost::is_any_of(" "));
                    for (auto it = std::begin(result); it != std::end(result);
                         ++it) {
                        int i = 0;
                        while (++i < it->size()) {
                            // if (std::isalpha(it->at(i))) {
                            //     break;
                            // }
                            if (it->at(i) == '#') {
                                changes.back()->addHashtags(it->substr(i));
                                break;
                            }
                        }
                    }
                }
            }
            if (cbyhit && attr_pair.name == "v") {
                cbyhit = false;
                changes.back()->addEditor(attr_pair.value);
            }
        }
    }
}
#endif // EOF LIBXML

std::string
fixString(std::string text)
{
    std::string newstr;
    int i = 0;
    while (i < text.size()) {
        if (text[i] == '\'') {
            newstr += "&apos;";
        } else if (text[i] == '\"') {
            newstr += "&quot;";
        } else if (text[i] == ')') {
            newstr += "&#41;";
        } else if (text[i] == '(') {
            newstr += "&#40;";
        } else if (text[i] == '\\') {
            // drop this character
        } else {
            newstr += text[i];
        }
        i++;
    }
    return boost::locale::conv::to_utf<char>(newstr, "Latin1");
}

} // namespace changeset