# ./CMakeLists.txt

# Cmake config
cmake_policy(SET CMP0167 NEW)
cmake_minimum_required(VERSION 3.16)

# Underpass project
project(Underpass VERSION 0.1 LANGUAGES CXX)

# C++ config
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Config headers
set(ETCDIR "${CMAKE_INSTALL_PREFIX}/etc")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h
)

include_directories(
    "${CMAKE_SOURCE_DIR}"
    /opt/homebrew/include # check
)

# Use only if absolutely needed (deprecated)
# Prefer linking libraries via find_package/pkg_check_modules
link_directories(
    /opt/homebrew/lib
    /usr/local/lib
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
#include(FindPkgConfig)

# Protozero
find_package(Protozero REQUIRED)
if(NOT PROTOZERO_FOUND)
    message(FATAL_ERROR "Protozero not found!\n")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX} -I${PROTOZERO_INCLUDE_DIR}")

# libxml++
pkg_check_modules(LIBXMLPP REQUIRED libxml++-3.0)
include_directories(${LIBXMLPP_INCLUDE_DIRS})

# libxml2
find_package(LibXml2 REQUIRED)
if(NOT LIBXML2_FOUND)
    message(FATAL_ERROR "libxml2 not found!")
endif()
include_directories(${LIBXML2_INCLUDE_DIRS})

# Enable libxml
add_compile_definitions(LIBXML)

# Underpass lib src
include_directories("${CMAKE_SOURCE_DIR}/src")
add_subdirectory(src)

# Underpass executable
add_executable(underpass src/underpass.cc)

# Link the underpass executable against underpass_lib
target_link_libraries(underpass PRIVATE underpass_lib)

# Compile options for the executable
target_compile_options(underpass PRIVATE -Wall -Wextra -fPIC -Wno-deprecated-declarations)

# Global properties for the executable
target_compile_definitions(underpass PRIVATE
    PKGLIBDIR="${CMAKE_INSTALL_PREFIX}/lib"
    SRCDIR="${CMAKE_SOURCE_DIR}"
    ETCDIR="${CMAKE_INSTALL_PREFIX}/etc"
    BOOST_LOCALE_HIDE_AUTO_PTR
    TIMING_DEBUG
)

