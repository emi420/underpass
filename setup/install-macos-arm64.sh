#!/bin/bash
#
# Copyright (c) 2024 Humanitarian OpenStreetMap Team
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
echo "underpass -i your_file.osm.pbf"
