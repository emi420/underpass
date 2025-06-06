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

#include <dejagnu.h>
#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <unistd.h>
#include <fcntl.h>

#include "utils/yaml.hh"
#include "utils/log.hh"

using namespace logger;

TestState runtest;

int
main(int argc, char *argv[])
{
    logger::LogFile &dbglogfile = logger::LogFile::getDefaultInstance();
    dbglogfile.setWriteDisk(true);
    dbglogfile.setLogFilename("yaml-test.log");
    dbglogfile.setVerbosity(3);

    yaml::Yaml yaml;
    std::string filespec = DATADIR;
    filespec += "/testsuite/libunderpass.all/test.yaml";
    yaml.read(filespec);

    if (yaml.get("tags").children.size() > 0) {
        runtest.pass("Yaml::get().children");
    } else {
        runtest.fail("Yaml::get().children");
        return 1;
    }

    if (yaml.contains_value("complete", "yes")) {
        runtest.pass("Yaml::get().contains_value()");
    } else {
        runtest.fail("Yaml::get().contains_value()");
        return 1;
    }
    if (!yaml.contains_value("complete", "foo")) {
        runtest.pass("Yaml::get().contains_value()");
    } else {
        runtest.fail("Yaml::get().contains_value()");
        return 1;
    }

    if (yaml.get("config").get("minangle").children.size() > 0) {
        runtest.pass("Yaml::get().get().children");
    } else {
        runtest.fail("Yaml::get().get().children");
        return 1;
    }

    if (yaml.contains_key("building:material") && yaml.contains_key("building:roof")) {
        runtest.pass("Yaml::contains_key()");
    } else {
        runtest.fail("Yaml::contains_key()");
        return 1;
    }

    if (yaml.contains_value("building:material", "metal") && yaml.contains_value("building:roof", "tiles")) {
        runtest.pass("Yaml::contains_value(good)");
    } else {
        runtest.fail("Yaml::contains_value(good)");
        return 1;
    }

    if (!yaml.contains_value("building:material", "metalx") && !yaml.contains_value("building:roof", "tilesy")) {
        runtest.pass("Yaml::contains_value(good)");
    } else {
        runtest.fail("Yaml::contains_value(good)");
        return 1;
    }

    if (yaml.contains_key("building:material") && yaml.contains_key("building:roof")) {
        runtest.pass("Yaml::containsKey()");
    } else {
        runtest.fail("Yaml::containsKey()");
        return 1;
    }

    if (yaml.contains_key("building:levels") && yaml.contains_key("building:levels")) {
        runtest.pass("Yaml::containsValue(none)");
    } else {
        runtest.fail("Yaml::containsValue(none)");
        return 1;
    }
}

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
