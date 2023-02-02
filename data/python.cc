//
// Copyright (c) 2020, 2021, 2022, 2023 Humanitarian OpenStreetMap Team
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

#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1

#include <boost/python.hpp>
#include "validate/hotosm.hh"
#include "validate/validate.hh"
#include "data/osmobjects.hh"
#include "validate/conflate.hh"
#include "data/pq.hh"

#include "log.hh"
using namespace logger;

#ifdef USE_PYTHON


BOOST_PYTHON_MODULE(underpass)
{
    using namespace boost::python;
    using namespace osmobjects;
    using namespace conflate;
    class_<OsmNode>("OsmNode")
        .def("setLatitude", &OsmNode::setLatitude)
        .def("setLongitude", &OsmNode::setLongitude)
        .def("setPoint", &OsmNode::setPoint)
        .def("addTag", &OsmNode::addTag)
        .def("dump", &OsmNode::dump);
    class_<OsmWay>("OsmWay")
        .def("isClosed", &OsmWay::isClosed)
        .def("numPoints", &OsmWay::numPoints)
        .def("getLength", &OsmWay::getLength)
        .def("addRef", &OsmWay::addRef)
        .def("addTag", &OsmWay::addTag)
        .def("dump", &OsmWay::dump);
    class_<OsmRelation>("OsmRelation")
        .def("addMember", &OsmRelation::addMember)
        .def("dump", &OsmRelation::dump);

    // 
    using namespace hotosm;
    class_<Hotosm, boost::noncopyable>("Validate")
        .def("checkTag", &Hotosm::checkTag)
        .def("checkWay", &Hotosm::checkWay)
        // .def("checkPOI", &Hotosm::checkPOI)
        .def("checkPOI", &Hotosm::_checkPOI, boost::python::return_value_policy<boost::python::manage_new_object>())
        .def("overlaps", &Hotosm::overlaps);

    class_<ValidateStatus, boost::noncopyable>("ValidateStatus")
        .def("hasStatus", &ValidateStatus::hasStatus)
        .def("dumpJSON", &ValidateStatus::dumpJSON)
        .def("dump", &ValidateStatus::dump);

#if 1
    class_<Conflate>("Conflate")
        .def("connect", &Conflate::connect)
        .def("newDuplicatePolygon", &Conflate::newDuplicatePolygon)
        .def("existingDuplicatePolygon", &Conflate::existingDuplicatePolygon)
        .def("newDuplicateLineString", &Conflate::newDuplicateLineString)
        .def("existingDuplicateLineString", &Conflate::existingDuplicateLineString);
#endif
}
#endif

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
