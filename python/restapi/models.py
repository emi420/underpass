#!/usr/bin/python3
#
# Copyright (c) 2023, 2024 Humanitarian OpenStreetMap Team
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

from pydantic import BaseModel
from typing import Union
    
class RawRequest(BaseModel):
    area: Union[str, None] = None
    tags: str = None
    hashtag: str = None
    dateFrom: str = None
    dateTo: str = None
    status: str = None
    orderBy: str = None
    page: int = None
    featureType: str = None

class StatsRequest(BaseModel):
    area: Union[str, None] = None
    tags: str = None
    hashtag: str = None
    dateFrom: str = None
    dateTo: str = None
    status: str = None
    featureType: str = None

