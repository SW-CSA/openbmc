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

SUMMARY = "MDIO access tool utilities"
DESCRIPTION = "Util for MDIO access"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;beginline=1;endline=21;md5=0bc745d8dcd774dd0dbaaa8e7f312ec1"

SRC_URI = "file://mdio-util.c \
		file://Makefile \
		file://LICENSE \
          "
S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 mdio-util ${D}${bindir}/mdio-util
}

FILES_${PN} = "${bindir}"
