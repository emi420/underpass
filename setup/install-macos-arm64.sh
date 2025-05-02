#!/bin/bash
#
# Copyright (c) 2024 Humanitarian OpenStreetMap Team
# Copyright (c) 2025 Emilio Mariscal
#
# This file is part of Underpass.
#
#     Underpass is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     Underpass is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with Underpass.  If not, see <https://www.gnu.org/licenses/>.

echo "Installing dependencies ..."
brew install \
    libtool \
    gdal \
    pkg-config \
    openssl \
    protobuf \
    libxml++3 \
    libpqxx \
    gumbo-parser \
    boost@1.85

cd ..

echo "Setting up build ..."
./autogen.sh
mkdir build ; cd build
../configure CXXFLAGS="-arch arm64 -g -O2" \
    LDFLAGS="-L/opt/homebrew/lib -L/usr/local/lib" \
    CPPFLAGS="-I/opt/homebrew/include" \
    --with-boost=/opt/homebrew/Cellar/boost/1.87.0

echo "Building ..."
make -j$(nproc) && sudo make install

echo "Done! now you may want to initialize the database with"
echo "underpass -s <DB> -i <PBF file path> -b <GeoJSON priority boundary>"
echo ""
echo "Example: "
echo "wget https://download.geofabrik.de/europe/andorra-latest.osm.pbf"
echo "wget https://download.geofabrik.de/europe/andorra.poly"
echo "python utils/poly2geojson.py andorra.poly"
echo "underpass -i andorra-latest.osm.pbf -s localhost/underpass -b andorra.geojson"
