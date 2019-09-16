# Copyright 2018-present Celestica. All Rights Reserved.
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

SUMMARY = "PSU EEPROM Utilities"
DESCRIPTION = "Util for PSU EEPROM Data Analysis"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://psu_fru.c;beginline=1;endline=4;md5=faeaad73a52d2feda98df6544667a813"

SRC_URI = "file://psu_fru.c \
		file://psu_fru.h \
		file://Makefile \
          "
S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 psu_fru ${D}${bindir}/psu_fru
}

FILES_${PN} = "${bindir}"
