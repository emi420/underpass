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

#include <iostream>
#include <dejagnu.h>
#include "osm/changeset.hh"
#include "osm/osmchange.hh"
#include <boost/geometry.hpp>
#include "utils/geoutil.hh"
#include "raw/queryraw.hh"
#include "raw/geobuilder.hh"

TestState runtest;

using namespace logger;

class TestChangeset : public changesets::ChangeSetFile {};

void
fillNodeCache(std::shared_ptr<osmchange::OsmChangeFile> &osmchange, geobuilder::GeoBuilder &geobuilder) {
    for (const auto& change : osmchange->changes) {
        for (const auto& node : change->nodes) {
            geobuilder.nodecache.insert(std::make_pair(node->id, node));
        }
    }
}

void
buildGeometries(std::shared_ptr<osmchange::OsmChangeFile> &osmchange, multipolygon_t &_poly) {
    auto db = std::make_shared<Pq>();
    std::shared_ptr<queryraw::QueryRaw> queryraw = std::make_shared<queryraw::QueryRaw>(db);
    geobuilder::GeoBuilder geobuilder(_poly, queryraw);
    fillNodeCache(osmchange, geobuilder);
    geobuilder.buildWays(osmchange);
    geobuilder.buildRelations(osmchange);
}

int
countFeatures(std::shared_ptr<osmchange::OsmChangeFile> &osmchange) {
    int nodeCount = 0;
    int wayCount = 0;
    int relCount = 0;
    for (const auto& change : osmchange->changes) {
        for (const auto& node : change->nodes) {
            if (node->priority) {
                nodeCount++;
            }
        }
        for (const auto& way : change->ways) {
            if (way->priority) {
                wayCount++;
            }
        }
        for (const auto& relation : change->relations) {
            if (relation->priority) {
                relCount++;
            }
        }
    }
    return nodeCount + wayCount + relCount;
}

int
countFeatures(TestChangeset &changeset) {
    return changeset.changes.size();
}

bool
getPriority(std::shared_ptr<osmchange::OsmChangeFile> &osmchange, bool debug = false) {
    bool result = true;
    for (const auto& change : osmchange->changes) {
        for (const auto& node : change->nodes) {
            if (!node->priority) {
                result = false;
            }
        }
        for (const auto& way : change->ways) {
            if (!way->priority) {
                result = false;
            }
        }
        for (const auto& relation : change->relations) {
            if (!relation->priority) {
                result = false;
            }
        }
    }
    return result;
}

int
main(int argc, char *argv[])
{
    multipolygon_t polyWholeWorld;
    boost::geometry::read_wkt("MULTIPOLYGON(((-180 90,180 90, 180 -90, -180 -90,-180 90)))", polyWholeWorld);
    multipolygon_t polySmallArea;
    boost::geometry::read_wkt("MULTIPOLYGON (((20 35, 45 20, 30 5, 10 10, 10 30, 20 35)))", polySmallArea);
    multipolygon_t polyHalf;
    boost::geometry::read_wkt("MULTIPOLYGON(((91.08473230447439 25.195528629552243,91.08475247411987 25.192143075605387,91.08932089882008 25.192152201214213,91.08927047470638 25.195501253482632,91.08473230447439 25.195528629552243)))", polyHalf);
    multipolygon_t polyHalfSmall;
    boost::geometry::read_wkt("MULTIPOLYGON(((91.08695983886719 25.195485830324174,91.08697056770325 25.192155906163805,91.08929872512817 25.192126781061106,91.08922362327574 25.195505246524604,91.08695983886719 25.195485830324174)))", polyHalfSmall);
    multipolygon_t polyEmpty;

    // -- ChangeSets

    // Small changeset in Bangladesh
    std::string changesetFile(DATADIR);
    changesetFile += "/testsuite/testdata/areafilter-test.osc";
    std::string osmchangeFile(DATADIR);
    osmchangeFile += "/testsuite/testdata/areafilter-test.osm";
    TestChangeset changeset;
    auto osmchange = std::make_shared<osmchange::OsmChangeFile>();
    changesets::ChangeSet *testChangeset;

    // ChangeSet - Whole world
    changeset.readChanges(changesetFile);
    
    changeset.areaFilter(polyWholeWorld);
    testChangeset = changeset.changes.front().get();
    if (testChangeset && testChangeset->priority) {
        runtest.pass("ChangeSet areaFilter - true (whole world)");
    } else {
        runtest.fail("ChangeSet areaFilter - true (whole world)");
        return 1;
    }

    // ChangeSet - Small area in North Africa
    // outside, not in priority area
    changeset.readChanges(changesetFile);
    changeset.areaFilter(polySmallArea);
    testChangeset = changeset.changes.front().get();
    if (testChangeset && testChangeset->priority) {
        runtest.fail("ChangeSet areaFilter - false (small area)");
    } else {
        runtest.pass("ChangeSet areaFilter - false (small area)");
    }

    // ChangeSet - Empty polygon
    // inside, in priority area
    changeset.readChanges(changesetFile);
    changeset.areaFilter(polyEmpty);
    testChangeset = changeset.changes.front().get();
    if (testChangeset && testChangeset->priority) {
        runtest.pass("ChangeSet areaFilter - true (empty)");
    } else {
        runtest.fail("ChangeSet areaFilter - true (empty)");
        return 1;
    }

    // ChangeSet - Half area
    changeset.readChanges(changesetFile);
    changeset.areaFilter(polyHalf);
    // inside, in priority area
    testChangeset = changeset.changes.front().get();
    if (testChangeset && testChangeset->priority) {
        runtest.pass("ChangeSet areaFilter - true (half area)");
    } else {
        runtest.fail("ChangeSet areaFilter - true (half area)");
        return 1;
    }

    // -- OsmChanges

    // // OsmChange - Empty poly
    osmchange->readChanges(osmchangeFile);

    // Build geometries
    buildGeometries(osmchange, polyEmpty);
 
    osmchange->areaFilter(polyEmpty);
    if (getPriority(osmchange) && countFeatures(osmchange) == 54) {
        runtest.pass("OsmChange areaFilter - 54 (empty poly)");
    } else {
        runtest.fail("OsmChange areaFilter - 54 (empty poly)");
        return 1;
    }

    // OsmChange - Whole world
    osmchange->areaFilter(polyWholeWorld);
    if (getPriority(osmchange) && countFeatures(osmchange) == 54) {
        runtest.pass("OsmChange areaFilter - 54 (whole world)");
    } else {
        std::cout << "Count: " << countFeatures(osmchange) << std::endl;
        runtest.fail("OsmChange areaFilter - 54 (Whole world)");
        return 1;
    }

    // OsmChange - Small area in North Africa
    // Outside priority area, count should be 0
    osmchange->areaFilter(polySmallArea);
    if (countFeatures(osmchange) == 0) {
        runtest.pass("OsmChange areaFilter - 0 (Small area)");
    } else {
        runtest.fail("OsmChange areaFilter - 0 (small area)");
        return 1;
    }

    // OsmChange - Small area in Bangladesh
    // 28 nodes / 5 ways / 1 relation inside priority area, count should be 34
    osmchange->areaFilter(polyHalf);
    if (countFeatures(osmchange) == 34) {
        runtest.pass("OsmChange areaFilter - 34 (small area)");
    } else {
        runtest.fail("OsmChange areaFilter - 34 (sSmall area)");
        return 1;
    }

    // OsmChange - Smaller area in Bangladesh
    // 12 nodes / 3 ways / 1 relation inside priority area, count should be 16
    osmchange->areaFilter(polyHalfSmall);
    if (countFeatures(osmchange) == 16) {
        runtest.pass("OsmChange areaFilter - 16 (smaller area)");
    } else {
        runtest.fail("OsmChange areaFilter - 16 (smaller area)");
        return 1;
    }

}

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

