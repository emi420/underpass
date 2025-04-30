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

#ifndef __PLANETINDEX_HH__
#define __PLANETINDEX_HH__

/// \file planetindex.hh
/// \brief This file parses a planet index file in HTML format

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <list>

#include <libxml++/libxml++.h>
#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

/// \namespace planetindex
namespace planetindex {

class PlanetIndex {

};

/// \class PlanetIndexFileFile
/// \brief This class manages an OSM planet index file.
class PlanetIndexFile : public xmlpp::SaxParser
{
  public:

    PlanetIndexFile(void){};
    PlanetIndexFile(const std::string &planetindex) {
            readIndex(planetindex);
    }

    /// Read a changeset file from disk or memory into internal storage
    bool readIndex(const std::string &planetindex);
    int getClosestIndex(const std::map<int, ptime>& indexes, const ptime& timestamp);
    std::string zeroPad(int number);

    /// Read an istream of the data and parse the HTML
    std::map<int, ptime> getIndexDateFromHTML(std::istream &html);

};

} // namespace planetindex

#endif // EOF __PLANETINDEX_HH__

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
