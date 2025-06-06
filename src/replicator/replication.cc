//
// Copyright (c) 2020, 2021, 2022, 2023, 2024 Humanitarian OpenStreetMap Team
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

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>
// #include <pqxx/pqxx>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
// #include <iterator>
#ifdef LIBXML
#include <libxml++/libxml++.h>
#endif
#include <gumbo.h>

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/visitor.hpp>

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/time_clock.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time.hpp>
#include <boost/format.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
using tcp = net::ip::tcp;         // from <boost/asio/ip/tcp.hpp>

#include "osm/changeset.hh"
#include "replicator/replication.hh"

/// Control access to the database connection
std::mutex db_mutex;

#include "utils/log.hh"
using namespace logger;

namespace replication {

/// Parse the two state files for a replication file, from
/// disk or memory.

/// There are two types of state files with of course different
/// formats for the same basic data. The simplest one is for
/// a changeset file. which looks like this:
///
/// \-\-\-
/// last_run: 2020-10-08 22:30:01.737719000 +00:00
/// sequence: 4139992
///
/// The other format is used for minutely change files, and
/// has more fields. For now, only the timestamp and sequence
/// number is stored. It looks like this:
///
/// \#Fri Oct 09 10:03:04 UTC 2020
/// sequenceNumber=4230996
/// txnMaxQueried=3083073477
/// txnActiveList=
/// txnReadyList=
/// txnMax=3083073477
/// timestamp=2020-10-09T10\:03\:02Z
///
/// State files are used to know where to start downloading files
StateFile::StateFile(const std::string &file, bool memory)
{
    std::string line;
    std::ifstream state;
    std::stringstream ss;

    // It's a disk file, so read it in.
    if (!memory) {
        if (!std::filesystem::exists(file)) {
                log_error("%1%  doesn't exist!", file);
            return;
        }
        try {
            state.open(file, std::ifstream::in);
        } catch (std::exception &e) {
            log_error(" opening %1% %2%", file, e.what());
            // return false;
        }
        // For a disk file, none of the state files appears to be larger than
        // 70 bytes, so read the whole thing into memory without
        // any iostream buffering.
        std::filesystem::path path = file;
        int size = std::filesystem::file_size(path);
        char *buffer = new char[size];
        state.read(buffer, size);
        ss << buffer;
        // FIXME: We do it this way to save lots of extra buffering
        // ss.rdbuf()->pubsetbuf(&buffer[0], size);
    } else {
        // It's in memory
        ss << file;
    }

    // Get the first line
    std::getline(ss, line, '\n');

    // This is a changeset state.txt file
    if (line == "---") {
        // Second line is the last_run timestamp
        std::getline(ss, line, '\n');
        // The timestamp is the second field
        std::size_t pos = line.find(" ");
        // 2020-10-08 22:30:01.737719000 +00:00
        timestamp = time_from_string(line.substr(pos + 1));

        // Third and last line is the sequence number
        std::getline(ss, line, '\n');
        pos = line.find(" ");
        // The sequence is the second field
        sequence = std::stol(line.substr(pos + 1));
        // This is a change file state.txt file
    } else {
        for (std::string line; std::getline(ss, line, '\n');) {
            std::size_t pos = line.find("=");

            // Not a key=value line. So we skip it.
            if (pos == std::string::npos) {
                continue;
            }

            const std::string key = line.substr(0, pos);
            const std::string value = line.substr(pos + 1);

            const std::vector<std::string> skipKeys{"txnMaxQueried", "txnActiveList", "txnReadyList", "txnMax"};
            if (key == "sequenceNumber") {
                sequence = std::stol(value);
            } else if (std::count(skipKeys.begin(), skipKeys.end(), key)) {
            } else if (key == "timestamp") {
                pos = value.find('\\', pos + 1);
                std::string tstamp = value.substr(0, pos); // get the date and the hour
                tstamp += value.substr(pos + 1, 3);        // get minutes
                pos = value.find('\\', pos + 1);
                tstamp += value.substr(pos + 1, 3); // get seconds
                timestamp = from_iso_extended_string(tstamp);
            } else {
                log_error("Invalid Key found: ", key);
            }
        }
    }

    state.close();
}

// Dump internal data to the terminal, used only for debugging
void
StateFile::dump(void)
{
    std::cout << "\t * * * * * " << std::endl;
    std::cerr << "\tDumping state.txt file" << std::endl;
    std::cerr << "\tTimestamp: " << timestamp << std::endl;
    std::cerr << "\tSequence: " << sequence << std::endl;
    std::cerr << "\tPath: " << path << std::endl;
    std::cout << "\t * * * * * " << std::endl;
}

bool
StateFile::isValid() const
{
    return timestamp != boost::posix_time::not_a_date_time &&
           sequence >= (frequency == frequency_t::changeset) ? 0 : 1 && !path.empty();
}

// parse a replication file containing changesets
bool
Replication::readChanges(const std::string &file)
{
    changesets::ChangeSetFile changeset;
    std::ifstream stream;
    stream.open(file, std::ifstream::in);
    changeset.readXML(stream);

    return true;
}

// Add this replication data to the changeset database
bool
Replication::mergeToDB()
{
    return false;
}

std::shared_ptr<std::vector<std::string>> &
Planet::getLinks(GumboNode *node, std::shared_ptr<std::vector<std::string>> &links)
{
    // if (node->type == GUMBO_NODE_TEXT) {
    //     std::string val = std::string(node->v.text.text);
    //     log_debug("FIXME: " << "GUMBO_NODE_TEXT " << val);
    // }

    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute *href;
        if (node->v.element.tag == GUMBO_TAG_A && (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
            // All the directories are a 3 digit number, and all the files
            // start with a 3 digit number
            if (href->value[0] >= 48 && href->value[0] <= 57) {
                //if (std::isalnum(href->value[0]) && std::isalnum(href->value[1])) {
                // log_debug("FIXME: %1%", href->value);
                if (std::strlen(href->value) > 0) {
                    links->push_back(href->value);
                }
            }
        }
        GumboVector *children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            getLinks(static_cast<GumboNode *>(children->data[i]), links);
        }
    }

    return links;
}

std::istringstream
Planet::processData(const std::string &dest, std::vector<unsigned char> &data)
{
    std::istringstream xml;
    try {
        {   // Scope to deallocate buffers
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            boost::iostreams::array_source arrs{reinterpret_cast<char const *>(data.data()), data.size()};
            inbuf.push(arrs);
            std::istream instream(&inbuf);
            xml.str(std::string{std::istreambuf_iterator<char>(instream), {}});
        }
    } catch (std::exception &e) {
        log_error("%1% is corrupted!", dest);
        std::cerr << e.what() << std::endl;
    }
    return xml;
}

std::istringstream
Planet::_processData(std::vector<unsigned char> &data)
{
    std::istringstream xml;
    try {
        {   // Scope to deallocate buffers
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
            boost::iostreams::array_source arrs{reinterpret_cast<char const *>(data.data()), data.size()};
            inbuf.push(arrs);
            std::istream instream(&inbuf);
            xml.str(std::string{std::istreambuf_iterator<char>(instream), {}});
        }
    } catch (std::exception &e) {
        log_error("File is corrupted!");
        std::cerr << e.what() << std::endl;
    }
    return xml;
}

// Download a file from planet
RequestedFile
Planet::downloadFile(const std::string &url, const std::string &destdir_base)
{

    RemoteURL remote(url);
    remote.destdir_base = destdir_base;
    RequestedFile file;
    std::string local_file_path = destdir_base + remote.filespec;

    if (std::filesystem::exists(local_file_path)) {
        file = readFile(local_file_path);
        // If local file doesn't work, remove it
        if (file.status == reqfile_t::localError) {
            std::filesystem::remove(local_file_path);
        }
        return file;
    }

    file.data = std::make_shared<std::vector<unsigned char>>();

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    ssl::stream<tcp::socket> stream{ioc, ctx};

    // Look up the domain name
    auto const results = resolver.resolve(remote.domain, std::to_string(port));
    try {
        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(stream.next_layer(), results.begin(), results.end());
        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);
    } catch (boost::system::system_error ex) {
        log_error("stream write failed: %1%", ex.what());
        file.status = reqfile_t::systemError;
        return file;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, url, version};

    req.keep_alive();

    // We want the host only: strip the rest
    static const std::regex re(R"raw(^(?:https?://)?([^/]+).*)raw");
    std::string host{remote.domain};
    host = std::regex_replace(host, re, "$1");

    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    // Send the HTTP request to the remote host
    try {
        http::write(stream, req);
        // log_debug("Downloading %1% ... ", url);
    } catch (boost::system::system_error ex) {
        log_error("stream write failed: %1%", ex.what());
        file.status = reqfile_t::systemError;
        return file;
    }

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;

    // Receive the HTTP response
    http::response_parser<http::string_body> parser;

    try {
        read(stream, buffer, parser);

        if (parser.get().result() == boost::beast::http::status::not_found ||
            parser.get().result() == boost::beast::http::status::gateway_timeout) {
            log_error("Remote file not found: %1%", url);
            file.status = reqfile_t::remoteNotFound;
            return file;
        } else {
            // Check the magic number of the file
            const auto is_gzipped{parser.get().body()[0] == 0x1f};
            std::shared_ptr<std::vector<unsigned char>> data;
            for (auto body = std::begin(parser.get().body()); body != std::end(parser.get().body()); ++body) {
                file.data->push_back(static_cast<unsigned char>(*body));
            }

            // Add the last newline back if not gzipped (or we'll get decompression error: unexpected end of file)
            if (!is_gzipped) {
                file.data->push_back('\n');
            }
        }

    } catch (boost::system::system_error ex) {
        log_error("stream read failed: %1%", ex.what());
    }

    // Gracefully close the stream
    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }

#ifdef USE_CACHE
    if (file.data->size() > 0) {
        writeFile(remote, file.data);
    } else {
        log_error("%1% does not exist!", remote.filespec);
    }
#endif
    file.status = reqfile_t::success;
    return file;
}

// Download a file from planet
RequestedFile
Planet::_downloadFile(const std::string &domain, const std::string &url)
{

    RequestedFile file;

    file.data = std::make_shared<std::vector<unsigned char>>();

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    ssl::stream<tcp::socket> stream{ioc, ctx};

    // Look up the domain name
    auto const results = resolver.resolve(domain, std::to_string(port));
    try {
        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(stream.next_layer(), results.begin(), results.end());
        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);
    } catch (boost::system::system_error ex) {
        log_error("stream write failed: %1%", ex.what());
        file.status = reqfile_t::systemError;
        return file;
    }

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, url, version};

    req.keep_alive();

    // We want the host only: strip the rest
    static const std::regex re(R"raw(^(?:https?://)?([^/]+).*)raw");
    std::string host{domain};
    host = std::regex_replace(host, re, "$1");

    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    // Send the HTTP request to the remote host
    try {
        http::write(stream, req);
        // log_debug("Downloading %1% ... ", url);
    } catch (boost::system::system_error ex) {
        log_error("stream write failed: %1%", ex.what());
        file.status = reqfile_t::systemError;
        return file;
    }

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;

    // Receive the HTTP response
    http::response_parser<http::string_body> parser;

    try {
        read(stream, buffer, parser);

        if (parser.get().result() == boost::beast::http::status::not_found ||
            parser.get().result() == boost::beast::http::status::gateway_timeout) {
            log_error("Remote file not found: %1%", url);
            file.status = reqfile_t::remoteNotFound;
            return file;
        } else {
            // Check the magic number of the file
            const auto is_gzipped{parser.get().body()[0] == 0x1f};
            std::shared_ptr<std::vector<unsigned char>> data;
            for (auto body = std::begin(parser.get().body()); body != std::end(parser.get().body()); ++body) {
                file.data->push_back(static_cast<unsigned char>(*body));
            }

            // Add the last newline back if not gzipped (or we'll get decompression error: unexpected end of file)
            if (!is_gzipped) {
                file.data->push_back('\n');
            }
        }

    } catch (boost::system::system_error ex) {
        log_error("stream read failed: %1%", ex.what());
    }

    // Gracefully close the stream
    beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }

    file.status = reqfile_t::success;
    return file;
}


RequestedFile
Planet::readFile(std::string &filespec) {
    log_debug("Reading cached file: %1%", filespec);
    // Since we want to read in the entire file so it can be
    // decompressed, blow off C++ streaming and just load the
    // entire thing.
    int size = 0;
    RequestedFile file;
    file.data = std::make_shared<std::vector<unsigned char>>();
    try {
        size = std::filesystem::file_size(filespec);
    } catch (const std::exception &ex) {
        log_error("File %1% doesn't exist but should!: %2%", filespec, ex.what());
        file.status = reqfile_t::localError;
        return file;
    }

    file.data->reserve(size);
    file.data->resize(size);
    int fd = open(filespec.c_str(), O_RDONLY);
    char *buf = new char[size];
    //memset(buf, 0, size);
    read(fd, buf, size);
    // FIXME: it would be nice to avoid this copy
    std::copy(buf, buf + size, file.data->begin());
    delete[] buf;
    close(fd);
    file.status = reqfile_t::success;
    return file;
}

void Planet::writeFile(RemoteURL &remote, std::shared_ptr<std::vector<unsigned char>> data) {
    std::string local_file_path = remote.destdir_base + remote.destdir;
    try {
        if (!std::filesystem::exists(local_file_path)) {
            std::filesystem::create_directories(local_file_path);
        }
    } catch (boost::system::system_error ex) {
        log_error("Destdir corrupted!: %1%, %2%", local_file_path, ex.what());
    }
    std::ofstream myfile;
    myfile.open(remote.destdir_base + remote.filespec, std::ofstream::out | std::ios::binary);
    myfile.write(reinterpret_cast<char *>(data.get()->data()), data.get()->size());
    myfile.flush();
    myfile.close();
    log_debug("Wrote downloaded file %1% to disk from %2%", remote.destdir_base + remote.filespec, remote.domain);
}

Planet::~Planet(void)
{
    ioc.restart(); // restart the I/O context
    // stream.shutdown();          // shutdown the socket used by the stream
}

Planet::Planet(void){
    // FIXME: for bulk downloads, we might want to strip across
    // all the mirrors. The support minutely diffs
    // pserver = "https://download.openstreetmap.fr";
    // pserver = "https://planet.maps.mail.ru";
    // pserver = "https://planet.openstreetmap.org";
    // connectServer();
};

// Dump internal data to the terminal, used only for debugging
void
Planet::dump(void)
{
    std::cerr << "Dumping Planet data" << std::endl;
    std::cerr << "\tDomain: " << domain << std::endl;
#if 0
    for (auto it = std::begin(changeset); it != std::end(changeset); ++it) {
        std::cerr << "Changeset at: " << it->first << it->second << std::endl;
    }
    for (auto it = std::begin(minute); it != std::end(minute); ++it) {
        std::cerr << "Minutely at: " << it->first << ": " << it->second << std::endl;
    }
    for (auto it = std::begin(hour); it != std::end(hour); ++it) {
        std::cerr << "Minutely at: " << it->first << ": " << it->second << std::endl;
    }
    for (auto it = std::begin(day); it != std::end(day); ++it) {
        std::cerr << "Daily at: " << it->first << ": " << it->second << std::endl;
    }
#endif
}

bool
Planet::connectServer(const std::string &planet)
{
    // Gracefully close the socket
    boost::system::error_code ec;
    ioc.restart();
    ctx.set_verify_mode(ssl::verify_none);
    // Strip off the https part
    std::string tmp;
    auto pos = planet.find(":");
    if (pos != std::string::npos) {
        tmp = planet.substr(pos + 3);
    } else {
        tmp = planet;
    }
    ssl::context ctx{ssl::context::sslv23_client};
    boost::asio::io_context ioc;

    try {
        tcp::resolver resolver{ioc};
        auto const dns = resolver.resolve(tmp, std::to_string(port));
        boost::asio::connect(stream.next_layer(), dns.begin(), dns.end(), ec);
        if (ec) {
            log_error("stream connect failed %1%", ec.message());
            return false;
        }
        stream.handshake(ssl::stream_base::client, ec);
        if (ec) {
            log_error("stream handshake failed %1%", ec.message());
            return false;
        }
    } catch (const std::exception &ex) {
        log_error("Connection to %1% failed: %2%", tmp, ex.what());
        return false;
    }

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    domain = planet;
    return true;
}

// Scan remote directory from planet
std::shared_ptr<std::vector<std::string>>
Planet::scanDirectory(const std::string &dir)
{

    RemoteURL remote(dir);
    log_debug("Scanning remote Directory: %1%", dir);

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    ssl::stream<tcp::socket> stream{ioc, ctx};

    // Look up the domain name
    auto const results = resolver.resolve(remote.domain, std::to_string(port));

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    auto links = std::make_shared<std::vector<std::string>>();

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, dir, version};
    req.set(http::field::host, remote.domain);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persistant
    boost::beast::flat_buffer buffer;

    // Receive the HTTP response
    http::response_parser<http::string_body> parser;
    // read_header(stream, buffer, parser);
    http::read(stream, buffer, parser);
    if (parser.get().result() == boost::beast::http::status::not_found) {
        return links;
    }
    GumboOutput *output = gumbo_parse(parser.get().body().c_str());
    getLinks(output->root, links);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    // Gracefully close the stream
    boost::beast::error_code ec;
    stream.shutdown(ec);
    if (ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if (ec) {
        log_debug("stream shutdown failed: %1%", ec.message());
    }
    return links;
}

RemoteURL::RemoteURL(void) : major(0), minor(0), index(0), frequency(minutely) {}

void
RemoteURL::parse(const std::string &rurl)
{
    if (rurl.empty()) {
        log_error("URL is empty!");
        return;
    }

    std::vector<std::string> parts;
    boost::split(parts, rurl, boost::is_any_of("/"));
    if (parts[0] != "https:") {
        datadir = parts[0];
        frequency = StateFile::freq_from_string(parts[1]);
        filespec = rurl.substr(rurl.find(parts[1]));
        major = std::stoi(parts[2]);
        minor = std::stoi(parts[3]);
        index = std::stoi(parts[4]);
        subpath = parts[2] + "/" + parts[3] + "/" + parts[4];
        destdir = datadir + "/" + parts[1] + "/" + parts[2] + "/" + parts[3];
    } else {
        if (parts.size() == 8) {
            domain = parts[2];
            datadir = parts[3];
            subpath = parts[5] + "/" + parts[6] + "/" + parts[7];
            try {
                frequency = StateFile::freq_from_string(parts[4]);
                major = std::stoi(parts[5]);
                minor = std::stoi(parts[6]);
                index = std::stoi(parts[7]);
            } catch (const std::exception &ex) {
                log_error("Error parsing URL: %1%", ex.what());
            }
            filespec = rurl.substr(rurl.find(datadir));
            destdir = datadir + "/" + parts[4] + "/" + parts[5] + "/" + parts[6];
        } else {
            log_error("Error parsing URL %1%: not in the expected form "
                "(https://<server>/replication/<frequency>/000/000/001)", rurl);
        }
    }
}

void
RemoteURL::updateDomain(const std::string &planet)
{
    domain = planet;
}

void
RemoteURL::updatePath(int _major, int _minor, int _index)
{
    boost::format majorfmt("%03d");
    boost::format minorfmt("%03d");
    boost::format indexfmt("%03d");

    major = _major;
    minor = _minor;
    index = _index;

    majorfmt % (_major);
    minorfmt % (_minor);
    indexfmt % (_index);
    std::size_t pos = filespec.rfind(".", filespec.size()-5);
    std::string suffix = filespec.substr(pos, filespec.size() - pos);

    std::vector<std::string> parts;
    boost::split(parts, filespec, boost::is_any_of("/"));
    std::string newpath = majorfmt.str() + "/" + minorfmt.str() + "/" + indexfmt.str();
    filespec = parts[0] + "/" + parts[1] + "/" + newpath + suffix;
    destdir = parts[0] + "/" + parts[1] + "/" + majorfmt.str() + "/" + minorfmt.str();
    subpath = newpath;
}

void
RemoteURL::increment(void)
{

    boost::format majorfmt("%03d");
    boost::format minorfmt("%03d");
    boost::format indexfmt("%03d");
    std::string newpath;

    if (minor == 999 && index == 999) {
        major++;
        minor = 0;
        index = 0;
    } else if (index == 999) {
        minor++;
        index = 0;
    } else {
        index++;
    }

    majorfmt % (major);
    minorfmt % (minor);
    indexfmt % (index);

    updatePath(major, minor, index);
}

void
RemoteURL::decrement(void)
{
    boost::format majorfmt("%03d");
    boost::format minorfmt("%03d");
    boost::format indexfmt("%03d");
    std::string newpath;

    if (minor == 000 && index == 000) {
        major--;
        minor = 999;
        index = 999;
    } else if (index == 000) {
        minor--;
        index = 999;
    } else {
        index--;
    }

    majorfmt % (major);
    minorfmt % (minor);
    indexfmt % (index);

    updatePath(major, minor, index);
}

RemoteURL &
RemoteURL::operator=(const RemoteURL &inr)
{
    domain = inr.domain;
    datadir = inr.datadir;
    subpath = inr.subpath;
    frequency = inr.frequency;
    major = inr.major;
    minor = inr.minor;
    index = inr.index;
    filespec = inr.filespec;
    destdir = inr.destdir;
    destdir_base = inr.destdir_base;

    return *this;
}

long
RemoteURL::sequence() const
{
    return major * 1000000 + minor * 1000 + index;
}

RemoteURL::RemoteURL(const RemoteURL &inr)
{
    domain = inr.domain;
    datadir = inr.datadir;
    subpath = inr.subpath;
    frequency = inr.frequency;
    major = inr.major;
    minor = inr.minor;
    index = inr.index;
    filespec = inr.filespec;
    destdir = inr.destdir;
    destdir_base = inr.destdir_base;
}

void
RemoteURL::dump(void)
{
    std::cerr << "{" << std::endl;
    std::cerr << "\t\"domain\": \"" << domain << "\"," << std::endl;
    std::cerr << "\t\"datadir\": \"" << datadir << "\"," << std::endl;
    std::cerr << "\t\"subpath\": \"" << subpath << "\"," << std::endl;
    std::cerr << "\t\"url\": \"" << getURL() << "\"," << std::endl;
    std::map<frequency_t, std::string> freqs;
    freqs[replication::minutely] = "minute";
    freqs[replication::hourly] = "hour";
    freqs[replication::daily] = "day";
    freqs[replication::changeset] = "changeset";
    std::cerr << "\t\"frequency\": " << (int)frequency << "," << std::endl;
    std::cerr << "\t\"major\": " << major << "," << std::endl;
    std::cerr << "\t\"minor\": " << minor << "," << std::endl;
    std::cerr << "\t\"index\": " <<  index << "," << std::endl;
    std::cerr << "\t\"filespec\": \"" << filespec << "\"," << std::endl;
    std::cerr << "\t\"destdir\": \"" << destdir_base + destdir << "\"" << std::endl;
    std::cerr << "}" << std::endl;

}

Planet::Planet(const RemoteURL &url)
{
    if (!connectServer(url.domain)) {
        // throw std::runtime_error("Error connecting to server " + url.domain);
    }
}


} // namespace replication

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:


