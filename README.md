# Underpass

Underpass **updates a local copy of the OSM database** in near real-time. 
It is designed to be **high performance** on modest hardware.

<img width="907" alt="Screenshot 2025-07-13 at 4 14 23 PM" src="https://github.com/user-attachments/assets/d1b95de1-b250-4165-b1bd-5f7c7e29e88d" />

## Product roadmap

✅ Done
⚙️ In progress

<!-- prettier-ignore-start -->
| Status | Feature |
|:--:| :-- |
|✅| Process OSM Planet replication files in near-real time |
|✅| Support Nodes, Ways and Relations |
|✅| Import OSM PBF files |
|✅| Fix and improve planet path calculation |
|✅| Refactor geometry builder |
|⚙️| **Migrate to CMake** |
| | Debian dist |
| | MacOS dist |
| | Python plugins support |

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

### Run

1. Download OSM PBF and GeoJSON priority boundary files
2. Run `underpass -s <DB> -i <PBF file path> -b <GeoJSON priority boundary>`

Example:

```bash
wget https://download.geofabrik.de/europe/andorra-latest.osm.pbf
wget https://download.geofabrik.de/europe/andorra.poly
python utils/poly2geojson.py andorra.poly
```

```bash
underpass -i andorra-latest.osm.pbf -s localhost/underpass -b andorra.geojson
```

If the process has stopped, you can continue from latest processed timestamp:

```bash
underpass -t latest -s localhost/underpass -b andorra.geojson
```

### Visualize data

Check these two projects if you want to request and visualize data easily:

* https://github.com/emi420/underpass-simple-api
* https://github.com/emi420/underpass-view


### License

Underpass is free software! you may use any Underpass project under the terms of
the GNU General Public License (GPL) Version 3.
