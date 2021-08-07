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
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "hottm/tmusers.hh"

namespace tmdb {

TMUser::TMUser(pqxx::result::const_iterator &row) {
    id = row.at("id").as(TaskingManagerIdType(0));
    username = row.at("username").as(std::string());
    name = row.at("name").as(std::string());
    role = static_cast<Role>(row.at("role").as(int(0)));
    gender = static_cast<Gender>(row.at("gender").as(int(0)));
    mapping_level =
        static_cast<MappingLevel>(row.at("mapping_level").as(int(0)));
    tasks_mapped = row.at("tasks_mapped").as(int(0));
    tasks_validated = row.at("tasks_validated").as(int(0));
    tasks_invalidated = row.at("tasks_invalidated").as(int(0));
    date_registered =
        time_from_string(row.at("date_registered").as(std::string()));
    last_validation_date =
        time_from_string(row.at("last_validation_date").as(std::string()));
    auto arr = row.at("projects_mapped").as_array();
    std::pair<pqxx::array_parser::juncture, std::string> elem;

    do {
        elem = arr.get_next();

        if (elem.first == pqxx::array_parser::juncture::string_value)
            projects_mapped.push_back(atoi(elem.second.c_str()));
    } while (elem.first != pqxx::array_parser::juncture::done);
}

bool
TMUser::operator==(const TMUser &other) const {
    return id == other.id && username == other.username && name == other.name &&
           gender == other.gender && mapping_level == other.mapping_level &&
           tasks_mapped == other.tasks_mapped &&
           tasks_invalidated == other.tasks_invalidated &&
           tasks_validated == other.tasks_validated &&
           date_registered == other.date_registered &&
           last_validation_date == other.last_validation_date &&
           projects_mapped == other.projects_mapped;
}

} // namespace tmdb
