//
// Copyright (c) 2025 Emilio Mariscal
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

#include "raw/queryraw.hh"
#include "data/pq.hh"

using namespace queryraw;

namespace rawtasker {

class RawTasker {
    public:

        RawTasker(std::shared_ptr<Pq> db, std::shared_ptr<QueryRaw> queryraw, int page_size, int concurrency);
        ~RawTasker(void){};

        void apply(OsmNode &osmNode);
        void apply(OsmWay &osmWay);
        void apply(OsmRelation &osmRelation);
        void finish();

    private:

        std::shared_ptr<Pq> db;
        std::shared_ptr<QueryRaw> queryraw;
        std::vector<OsmNode> nodecache;
        std::vector<OsmWay> waycache;
        std::vector<OsmRelation> relcache;
        int page_size;
        int concurrency;
        int chunk_size;
        int stat_nodes = 0;
        int stat_ways = 0;
        int stat_rels = 0;
        void checkNodes(bool finish);
        void checkWays(bool finish);
        void checkRelations(bool finish);
        void checkNodes();
        void checkWays();
        void checkRelations();
        void threadWayProcess(std::vector<OsmWay*> ways);
        void threadNodeProcess(std::vector<OsmNode*> nodes);
        void threadRelationProcess(std::vector<OsmRelation*> rels);


};

}
