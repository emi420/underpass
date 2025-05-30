//
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

#include <string>
#include <vector>
#include <iostream>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/filesystem.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <locale>
#include <regex>
#include <iomanip>
#include <sstream>
#include "replicator/planetindex.hh"
#include "utils/log.hh"
using namespace logger;

#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <boost/filesystem.hpp>
    namespace fs = boost::filesystem;
#endif

namespace planetindex {

// Read a planet index HTML file and parse index and date
bool
PlanetIndexFile::readIndex(const std::string &file)
{
    setlocale(LC_ALL, "");
    std::ifstream change;
    int size = 0;
    unsigned char *buffer;
    log_debug("Reading Planet Index file %1%", file);
    std::string suffix = fs::path(file).extension();
    std::ifstream ifile(file, std::ios_base::in | std::ios_base::binary);
    change.open(file, std::ifstream::in);
    getIndexDateFromHTML(change);
    return true;
}

// Function to parse custom date format ("13-Mar-2025 21:14")
bool parse_custom_date(const std::string& date_str, ptime& pt) {
    // Define the input format for the custom date string
    std::istringstream ss(date_str);
    ss.imbue(std::locale(ss.getloc(), new time_input_facet("%d-%b-%Y %H:%M")));

    ss >> pt; // Parse the date string into a ptime object

    return !pt.is_not_a_date_time(); // Check if parsing was successful
}

// Function to parse ISO 8601 extended format ("2014-08-11 13:22")
bool parse_iso_date(const std::string& date_str, ptime& pt) {
    // Define the input format for the ISO date string
    std::istringstream ss(date_str);
    ss.imbue(std::locale(ss.getloc(), new time_input_facet("%Y-%m-%d %H:%M")));

    ss >> pt; // Parse the date string into a ptime object

    return !pt.is_not_a_date_time(); // Check if parsing was successful
}

// Receives HTML with a list of folders and datetimes,
// returns a map of remote indexes and datetimes
std::map<int, ptime>
PlanetIndexFile::getIndexDateFromHTML(std::istream &html)
{
    // This regexpr extracts index and datetime from strings like these:
    // <img src="/icons/folder.gif" alt="[DIR]"> <a href="006/">006/</a>  2025-03-13 21:14
    // <a href="006/">006/</a>  13-Mar-2025 21:14
    // <img src="/icons/text.gif" alt="[TXT]"> <a href="647.state.txt">647.state.txt</a> 2025-03-28 12:26
    // <a href="000.osc.gz">000.osc.gz</a> 28-Mar-2025 01:27
    std::regex pattern(R"(<a href="([^/]+/|[^"]+\.osc\.gz|[^"]+\.txt)\">([^<]+)</a>\s+((?:\d{4}-\d{2}-\d{2} \d{2}:\d{2})|(?:\d{2}-[A-Za-z]{3}-\d{4} \d{2}:\d{2})))");

    std::string line;
    std::map<int, ptime> result;
    while (std::getline(html, line)) {
        std::smatch matches;
        try {
            if (std::regex_search(line, matches, pattern)) {
                auto index = stoi(matches[1]);
                auto date = matches[3];
                ptime dt;
                if (!parse_iso_date(date, dt)) {
                    parse_custom_date(date, dt);
                }

                result.insert(std::pair(index, dt));
            }
        } catch(std::exception const& e) {
            log_error("Invalid string %1%", line);
        }
    }

    return result;
}

std::string 
PlanetIndexFile::zeroPad(int number) {
    std::ostringstream stream;
    stream << std::setw(3) << std::setfill('0') << number;
    return stream.str();
}

// Receives a map (remote indexes, datetime)
// and a datetime, return the closest index
int 
PlanetIndexFile::getClosestIndex(const std::map<int, boost::posix_time::ptime>& indexes, const boost::posix_time::ptime& timestamp) {
    if (indexes.empty()) {
        throw std::invalid_argument("Indexes map is empty");
    }

    int folderIndex = -1;

    auto it = indexes.begin();
    auto nextIt = std::next(it);
    boost::posix_time::ptime prevStartDatetime;
    boost::posix_time::ptime prevEndDatetime;

    // Iterate over each folder and check if the timestamp is in the range
    while (nextIt != indexes.end()) {
        const boost::posix_time::ptime& folderStart = it->second;
        const boost::posix_time::ptime& folderEnd = nextIt->second;

        if (
            (timestamp >= prevStartDatetime && timestamp <= prevEndDatetime)
            || (timestamp < folderStart)
         ) {
            folderIndex = it->first;
            break;
        }

        prevStartDatetime = folderStart;
        prevEndDatetime = folderEnd;
        it = nextIt;
        nextIt = std::next(it);

        // If no index found, return last
        if (nextIt == indexes.end()) {
            folderIndex = it->first;
        }
    }

    // If the timestamp is after the last folder's timestamp
    if (folderIndex == -1 && timestamp >= it->second) {
        folderIndex = it->first;
    }

    if (folderIndex == -1) {
        return 0;
    }

    return folderIndex;

}


}