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

### License

Underpass is free software! you may use any Underpass project under the terms of
the GNU General Public License (GPL) Version 3.
