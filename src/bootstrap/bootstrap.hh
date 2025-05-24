//
// Copyright (c) 2023, 2024 Humanitarian OpenStreetMap Team
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
#include "underpassconfig.hh"

using namespace queryraw;

namespace bootstrap {

class Bootstrap {
  public:
    Bootstrap(void);
    ~Bootstrap(void){};

    ///
    /// \brief start Starts bootstrapping process
    /// \param config is the Underpass config
    /// \return void
    ///
    void start(const underpassconfig::UnderpassConfig &config);
    boost::posix_time::ptime getLatestTimestamp(void);
    void initializeDB(void);
    void createDBIndexes(void);

    private:
      std::shared_ptr<Pq> db;
      std::shared_ptr<QueryRaw> queryraw;

      bool connect(const std::string &db_url);
      void processPBF(std::string &pbf, int page_size, int concurrency);

  };

}
