# Build from source

## Linux

Tested in Ubuntu 22.04.5 LTS

### Install dependencies

```sh
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

```sh
$ ./autogen.sh && \
  mkdir build && cd build && \ 
  ../configure && make -j$(nproc) && sudo make install
```

## MacOS

### Install dependencies

```
sudo port install boost
brew install \
    libtool \
    gdal \
    pkg-config \
    openssl \
    protobuf \
    libxml++3 \
    libpqxx \
    gumbo-parser
```

### Build (Intel)

```sh
sh ./autogen.sh
mkdir build && cd build
../configure CXXFLAGS=" \
    -I/usr/local/include \
    -L/opt/homebrew/lib \
    -I/opt/homebrew/include \
    -I/opt/homebrew/Cellar \
    -L/usr/local/Cellar" CXX="g++"
make -j$(nproc) && sudo make install
```

## MacOS (Silicon)

The process is similar to Intel but adding `-arch arm64` to `CXXFLAGS`:

```sh
sh ./autogen.sh
mkdir build && cd build
../configure CXXFLAGS=" -arch arm64\
    -I/usr/local/include \
    -L/opt/homebrew/lib \
    -I/opt/homebrew/include \
    -I/opt/homebrew/Cellar \
    -L/usr/local/Cellar" CXX="g++"
make -j$(nproc) && sudo make install
```
