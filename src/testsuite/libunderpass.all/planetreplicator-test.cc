//
// Copyright (c) 2020, 2021, 2022, 2023, 2024 Humanitarian OpenStreetMap Team
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

#include <dejagnu.h>
#include <iostream>
#include <vector>
#include "utils/log.hh"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/program_options.hpp>
#include "boost/format.hpp"
#include "osm/osmchange.hh"
#include "underpassconfig.hh"
#include "replicator/planetreplicator.hh"
#include "osm/changeset.hh"
#include "replicator/replication.hh"
#include <string>
#include "utils/yaml.hh"
#include <iterator>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>


using namespace logger;
using namespace underpassconfig;
using namespace planetreplicator;
using namespace replication;
using namespace boost::posix_time;

namespace opts = boost::program_options;

class TestCO : public osmchange::OsmChangeFile {
};

class TestPlanet : public replication::Planet {
};

TestState runtest;

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

std::string fetch_url_content(const std::string& host, const std::string& port, const std::string& target) {
    try {
        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        // Convert the response body to a string
        return beast::buffers_to_string(res.body().data());
    }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
}

void testPath(underpassconfig::UnderpassConfig config) {
    planetreplicator::PlanetReplicator replicator;
    auto osmchange = replicator.findRemotePath(config, config.start_time);
    TestCO change;
    std::cout <<  osmchange->filespec << std::endl;
    if (std::filesystem::exists(osmchange->filespec)) {
        change.readChanges(osmchange->filespec);
    } else {
        TestPlanet planet;
        auto data = planet.downloadFile(osmchange->getURL()).data;
        auto xml = planet.processData(osmchange->filespec, *data);
        std::istream& input(xml);
        change.readXML(input);
    }

    ptime timestamp = change.changes.back()->final_entry;
    auto timestamp_string_debug = to_simple_string(timestamp);
    auto start_time_string_debug = to_simple_string(config.start_time);

    time_duration delta = timestamp - config.start_time;
    if (delta.hours() > -5 && delta.minutes() < 5) {
        runtest.pass("Find remote path from timestamp +/- 5 hour (" + start_time_string_debug + ") (" + timestamp_string_debug + ")");
    } else {
        runtest.fail("Find remote path from timestamp +/- 5 hour (" + start_time_string_debug + ") (" + timestamp_string_debug + ")");
    }
}

int
main(int argc, char *argv[]) {

    logger::LogFile &dbglogfile = logger::LogFile::getDefaultInstance();
    dbglogfile.setWriteDisk(true);
    dbglogfile.setLogFilename("planetreplicator-test.log");
    dbglogfile.setVerbosity(3);

    UnderpassConfig config;
    config.planet_server = config.planet_servers[0].domain + "/replication";

    // Declare the supported options.
    opts::positional_options_description p;
    opts::variables_map vm;
    opts::options_description desc("Allowed options");
    desc.add_options()
        ("timestamp,t", opts::value<std::vector<std::string>>(), "Starting timestamp")
    ;
    opts::store(opts::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    opts::notify(vm);

    // Example: planetreplicator-test -t "2014-07-12 06:00:24"
    if (vm.count("timestamp")) {
        auto timestamps = vm["timestamp"].as<std::vector<std::string>>();
        config.start_time = time_from_string(timestamps[0]);
        testPath(config);
    } else {
        std::vector<std::string> dates = {
            "-01-01 00:04:01",
            "-03-07 03:01:40",
            "-06-12 20:21:20",
            "-08-17 10:33:30",
            "-10-22 08:45:00",
            "-12-28 17:30:06",
        };

        ptime now = boost::posix_time::microsec_clock::universal_time();
        int next_year = now.date().year();

        for (int i = 2023; i != next_year + 1; ++i) {
            for (auto it = std::begin(dates); it != std::end(dates); ++it) {

                std::string year_string = std::to_string(i);
                std::string ts(year_string + *it);
                config.start_time = time_from_string(ts);

                time_duration diffWithNow = now - config.start_time;

                if (diffWithNow.hours() > 0) {
                    testPath(config);
                }
            }
        }
    }

}

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
