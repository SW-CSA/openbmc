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

SUMMARY = "EEPROM tool Utilities"
DESCRIPTION = "Util for EEPROM tool"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sys_eeprom.c;beginline=2;endline=14;md5=1f84bdc9ce4c5a55e83a42b658a763a7"

SRC_URI = "file://sys_eeprom.c \
		file://onie_tlvinfo.c \
		file://sys_eeprom_sysfs_file.c \
		file://crc32.c \
		file://onie_tlvinfo.h \
		file://sys_eeprom.h \
		file://Makefile \
          "
S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 syseeprom ${D}${bindir}/syseeprom
}

FILES_${PN} = "${bindir}"
