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
#

SUMMARY = "EEPROM Bin tool Utilities"
DESCRIPTION = "Util for EEPROM Bin tool"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=1;endline=21;md5=0bc745d8dcd774dd0dbaaa8e7f312ec1"

SRC_URI = "file://eeprom-bin.c \
		file://fru-defs.h \
		file://fru.conf.sample \
		file://Makefile \
		file://LICENSE \
		file://lib \
          "
S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -d ${D}"/tmp"
	install -m 0755 eeprom-bin ${D}${bindir}/eeprom-bin
	install -m 0644 fru.conf.sample ${D}"/tmp"/fru.conf.sample
}

FILES_${PN} = "${bindir} /tmp"
