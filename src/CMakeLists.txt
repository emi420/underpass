# ./src/CMakeLists.txt

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

find_package(PkgConfig REQUIRED)

find_package(Boost REQUIRED COMPONENTS
  date_time system filesystem log program_options thread regex serialization locale timer
)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost libraries not found!")
endif()

pkg_check_modules(PQXX REQUIRED libpqxx)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.68)
pkg_check_modules(LIBXMLPP REQUIRED libxml++-3.0)

find_package(Protozero REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Osmium REQUIRED)

add_library(underpass_lib
    utils/log.cc utils/log.hh
    dsodefs.hh gettext.h
    underpassconfig.hh
    raw/queryraw.cc raw/queryraw.hh
    raw/geobuilder.cc raw/geobuilder.hh
    osm/changeset.cc osm/changeset.hh
    osm/osmchange.cc osm/osmchange.hh
    osm/osmobjects.cc osm/osmobjects.hh
    replicator/replication.cc replicator/replication.hh
    replicator/planetreplicator.cc replicator/planetreplicator.hh
    replicator/planetindex.cc replicator/planetindex.hh
    replicator/threads.cc replicator/threads.hh
    bootstrap/bootstrap.cc bootstrap/bootstrap.hh
    bootstrap/rawtasker.cc bootstrap/rawtasker.hh
    bootstrap/osmprocessor.cc bootstrap/osmprocessor.hh
    utils/geoutil.cc utils/geoutil.hh
    utils/yaml.cc utils/yaml.hh
    data/pq.cc data/pq.hh
    data/utils.cc data/utils.hh
)

target_link_libraries(underpass_lib
  PRIVATE
    ${Boost_LIBRARIES}
    ${PQXX_LIBRARIES}
    ${GLIBMM_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXMLPP_LIBRARIES}
)

target_include_directories(underpass_lib
  PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${PQXX_INCLUDE_DIRS}
    ${GLIBMM_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
    ${LIBXMLPP_INCLUDE_DIRS}
    ${PROTOZERO_INCLUDE_DIR}
    ${LIBOSMIUM_INCLUDE_DIR}
)

target_compile_options(underpass_lib PRIVATE -Wall -Wextra -fPIC)
