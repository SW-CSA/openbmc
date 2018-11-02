# Copyright 2015-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://mTerm/run \
            file://sol.sh \
			file://tty_helper.h \
			file://mTerm_helper.h \
			file://mTerm_helper.c \
           "

S = "${WORKDIR}"

# Go with default names of mTerm for MTERM_SERVICES
# since we have just one console.

do_install_append() {
    install -d ${D}/usr/local/bin
    install -m 755 sol.sh ${D}/usr/local/bin/sol.sh
}

FILES_${PN} += "/usr/local/bin"