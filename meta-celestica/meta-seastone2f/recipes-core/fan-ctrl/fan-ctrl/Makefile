# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

all: fand fand_v2 fand32_v2 fand_phalanx

fand: fand.cpp watchdog.cpp
	$(CXX) $(CXXFLAGS) -pthread -o $@ $^ $(LDFLAGS)

fand_v2: fand_v2.cpp watchdog.cpp
	$(CXX) $(CXXFLAGS) -pthread -o $@ $^ $(LDFLAGS)

fand32_v2: fand32_v2.cpp watchdog.cpp
	$(CXX) $(CXXFLAGS) -pthread -o $@ $^ $(LDFLAGS)

fand_phalanx: fand_phalanx.cpp watchdog.cpp
	$(CXX) $(CXXFLAGS) -pthread -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o fand fand_v2 fand32_v2 fand_phalanx
