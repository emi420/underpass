//
// Copyright (c) 2020, 2021, 2022 Humanitarian OpenStreetMap Team
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

#ifndef __STATSCONFIG_HH__
#define __STATSCONFIG_HH__

/// \file statsconfig.hh
/// \brief Simple YAML file reader.
///
/// Read in a YAML config file and create a nested data structure so it can be accessed.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <map>
#include <string>
#include <vector>
#include <memory>
#include "galaxy/osmchange.hh"

/// \namespace statsconfig
namespace statsconfig {

    class StatsConfig {
        public:
            std::string name;
            std::map<std::string, std::vector<std::string>> way;
            std::map<std::string, std::vector<std::string>> node;
            std::map<std::string, std::vector<std::string>> relation;
            StatsConfig(std::string name);
            StatsConfig
            (
                std::string name,
                std::map<std::string, std::vector<std::string>> way,
                std::map<std::string, std::vector<std::string>> node,
                std::map<std::string, std::vector<std::string>> relation
            );
    };

    class StatsConfigFile {
        public:
            static std::shared_ptr<std::vector<statsconfig::StatsConfig>> read_yaml(std::string filename);
    };

    class StatsConfigSearch {
        public:
            static std::string tag_value(std::string tag, std::string value, osmchange::osmtype_t type, std::shared_ptr<std::vector<StatsConfig>> statsconfig);
            static bool category(std::string tag, std::string value, std::map<std::string, std::vector<std::string>> tags);
    };

} // EOF statsconfig namespace

#endif  // EOF __STATSCONFIG_HH__

// Local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
