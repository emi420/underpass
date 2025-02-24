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

#ifndef __PLANETREPLICATOR_HH__
#define __PLANETREPLICATOR_HH__

/// \file planetreplicator.hh
/// \brief This file is used to indentify, download and process replication files
///
/// Identifies, downloads, and processes replication files.
/// Replication files are available from the OSM planet server.

// This is generated by autoconf
#ifdef HAVE_CONFIG_H
#include "unconfig.h"
#endif

#include "underpassconfig.hh"
#include "replicator/replication.hh"
#include "osm/changeset.hh"
#include <vector>
#include <memory>

using namespace underpassconfig;

/// \namespace planetreplicator
namespace planetreplicator {

class PlanetReplicator : public replication::Planet {
    public:
        PlanetReplicator(void);
        ~PlanetReplicator(void) {};
        bool initializeRaw(std::vector<std::string> &rawfile, const std::string &database);
        std::shared_ptr<RemoteURL> findRemotePath(const underpassconfig::UnderpassConfig &config, ptime time);
    // These are used for the import command
    private:
        std::vector<StateFile> default_minutes;
        std::vector<StateFile> default_changesets;
        std::shared_ptr<changesets::ChangeSetFile> changes;  ///< All the changes in the file
        std::shared_ptr<std::map<std::string, int>> hashes; ///< Existing hashtags
};

} // namespace planetreplicator

#endif // EOF __PLANETREPLICATOR_HH__

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
