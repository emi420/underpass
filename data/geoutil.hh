//
// Copyright (c) 2020, 2021 Humanitarian OpenStreetMap Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __GEOUTIL_HH__
#define __GEOUTIL_HH__

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
# include "unconfig.h"
#endif

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <pqxx/pqxx>

#include <boost/date_time.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
using namespace boost::posix_time;
using namespace boost::gregorian;
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>

#include "osmstats/osmstats.hh"
#include "data/osmobjects.hh"

/// \file geoutil.hh
/// \brief This file parses a data file containing boundaries
///
/// This uses GDAL to read and parse a file in any supported format
/// and loads it into a data structure. This is used to determine which
///country a change was made in.

/// \namespace geoutil
namespace geoutil {

/// \class GeoUtil
/// \brief Read in the priority area boundaries data file
///
/// This parses the data file in any GDAL supported format into a data
/// structure that can be used to determine which area a change was
/// made in. This is used from C++ to avoid a call to the database
/// when processing changes.
class GeoUtil
{
public:
    GeoUtil(void) {
        GDALAllRegister();
    };
    /// Read a file into internal storage so boost::geometry functions
    /// can be used to process simple geospatial calculations instead
    /// of using postgres. This data is is used to determine which
    /// country a change was made in, or filtering out part of the
    /// planet to reduce data size. Since this uses GDAL, any
    /// multi-polygon file of any supported format can be used.
    bool readFile(const std::string &filespec);
    
    /// See if this changeset is in a priority area. We ignore changsets in
    /// areas like North America to reduce the amount of data needed
    /// for calulations. This boundary can always be modified.
    bool inPriorityArea(double lat, double lon) {
        return inPriorityArea(point_t(lon, lat));
    };

    bool inPriorityArea(polygon_t poly) {
        point_t pt;
        boost::geometry::centroid(poly, pt);
        return inPriorityArea(pt);
    };
    bool inPriorityArea(point_t pt) {
        return boost::geometry::within(pt, boundary);
    };

// private:
    multipolygon_t boundary;
};
    
}       // EOF geoutil

#endif  // EOF __GEOUTIL_HH__
