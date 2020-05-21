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

DEPENDS_append = " update-rc.d-native"

LIC_FILES_CHKSUM = "file://fand.cpp;beginline=6;endline=18;md5=219631f7c3c1e390cdedab9df4b744fd"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://get_fan_speed.sh \
            file://set_fan_speed.sh \
            file://setup-fan.sh \
            file://fand.cpp \
            file://fand_v2.cpp \
            file://fand32_v2.cpp \
            file://fand_phalanx.cpp \
            file://fand_v2.h \
            file://watchdog.cpp \
            file://watchdog.h \
            file://Makefile \
            file://pid_config.ini \
            file://pid_config_v2.ini \
            file://pid_config_v2_phalanx.ini \
           "

S = "${WORKDIR}"

binfiles = "                                    \
    get_fan_speed.sh                            \
    set_fan_speed.sh                            \
    fand                                        \
    fand_v2                                     \
    fand32_v2                                   \
    fand_phalanx                                   \
    "


do_install() {
  bin="${D}/usr/local/bin"
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${bin}/$f
  done
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 644 pid_config.ini ${D}${sysconfdir}/pid_config.ini
  install -m 644 pid_config_v2.ini ${D}${sysconfdir}/pid_config_v2.ini
  install -m 644 pid_config_v2_phalanx.ini ${D}${sysconfdir}/pid_config_v2_phalanx.ini
  install -m 755 setup-fan.sh ${D}${sysconfdir}/init.d/setup-fan.sh
  update-rc.d -r ${D} setup-fan.sh start 125 2 3 4 5 .
}

FILES_${PN} += "${sysconfdir}"
