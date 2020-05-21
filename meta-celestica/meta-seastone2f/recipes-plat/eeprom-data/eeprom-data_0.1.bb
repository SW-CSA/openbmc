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

SUMMARY = "EEPROM Data Utilities"
DESCRIPTION = "Util for EEPROM Data Analysis"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://license.txt;beginline=1;endline=4;md5=3294b37156963949d5f121a9c7514e43"

SRC_URI = "file://eeprom_data.c \
        file://eeprom_data.h \
        file://ezxml.c \
        file://ezxml.h \
        file://Makefile \
        file://license.txt \
        file://eeprom.xml \
        file://eeprom_fishbone.xml \
        file://eeprom_phalanx.xml \
          "
S = "${WORKDIR}"
pkgdir = "eeprom-data"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 eeprom_data ${D}${bindir}/eeprom_data
    install -d ${D}${sysconfdir}/eeprom-data
    install -m 644 eeprom.xml ${D}${sysconfdir}/eeprom-data/eeprom.xml
    install -m 644 eeprom_fishbone.xml ${D}${sysconfdir}/eeprom-data/eeprom_fishbone.xml
    install -m 644 eeprom_phalanx.xml ${D}${sysconfdir}/eeprom-data/eeprom_phalanx.xml
    install -d ${D}${sysconfdir}/init.d
}

FILES_${PN} = "${bindir} ${pkgdir} ${sysconfdir}"
