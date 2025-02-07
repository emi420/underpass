# Underpass

Underpass **updates a local copy of the OSM database** in near real-time. 
It is designed to be **high performance** on modest hardware.

## Getting started

### Install dependencies

```
sudo apt-get update \
    && apt-get install -y software-properties-common \
    && apt-get update && apt-get install -y \
        libboost-dev \
        autotools-dev \
        swig \
        pkg-config \
        gcc \
        build-essential \
        ccache \
        libboost-all-dev \
        dejagnu \
        libjemalloc-dev \
        libxml++2.6-dev \
        doxygen \
        libgdal-dev \
        libosmium2-dev \
        libpqxx-dev \
        postgresql \
        libgumbo-dev \
        librange-v3-dev
```
### Build

```bash
./autogen.sh && \
  mkdir build && cd build && \ 
  ../configure && make -j$(nproc) && sudo make install
```

### Setup DB

1. Import `setup/db/underpass.sql` to a PostGIS database
2. Download a OSM OBF file, ex: `wget https://download.geofabrik.de/south-america/uruguay-latest.osm.pbf`
3. Import it `underpass -i uruguay-latest.osm.pbf`

### Setup priority area

If you already have a GeoJSON, skip steps 1 and 2.

1. Download a poly file, ex: `wget https://download.geofabrik.de/south-america/uruguay.poly`
2. Convert it to GeoJSON, ex: `python utils/poly2geojson.py uruguay.poly`
3. Copy priority file, ex: `cp uruguay.poly /usr/local/etc/underpass/priority.geojson`

### Run

`underpass -u <OSM Planet path, ex: 006/390/702>`

### Visualize data

Check these two projects if you want to request and visualize data easily:

* https://github.com/emi420/underpass-simple-api
* https://github.com/emi420/underpass-view


### License

Underpass is free software! you may use any Underpass project under the terms of
the GNU General Public License (GPL) Version 3.
