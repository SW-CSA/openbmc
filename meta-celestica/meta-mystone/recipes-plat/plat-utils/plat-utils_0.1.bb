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
SUMMARY = "Utilities"
DESCRIPTION = "Various utilities"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

SRC_URI = "file://ast-functions \
           file://setup_i2c.sh \
           file://setup_pca9506.sh \
           file://setup_isl68137.sh \
           file://setup_module.sh \
           file://rsyslog_config.sh \
           file://wedge_power.sh \
           file://board-utils.sh \
           file://power-on.sh \
           file://bcm5387.sh \
           file://setup_platform.sh \
           file://setup_sensors.sh \
           file://fru-util \
           file://come_power.sh \
           file://mount_emmc.sh \
           file://us_monitor.sh \
           file://start_us_monitor.sh \
           file://power_monitor.py \
           file://version_dump \
           file://led_location.sh \
           file://rc.local \
           file://power \
           file://led_location \
           file://COPYING \
          "

pkgdir = "utils"

S = "${WORKDIR}"

binfiles = " \
  "

DEPENDS_append = "update-rc.d-native"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  localbindir="/usr/local/bin"
  install -d ${D}${localbindir}
  install -d $dst
  install -m 644 ast-functions ${dst}/ast-functions

  # init
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}/mnt

  install -m 0755 bcm5387.sh ${D}${localbindir}/bcm5387.sh
  install -m 0755 wedge_power.sh ${D}${localbindir}/wedge_power.sh
  install -m 0755 board-utils.sh ${D}${localbindir}/board-utils.sh
  install -m 755 setup_pca9506.sh ${D}${localbindir}/setup_pca9506.sh
  install -m 755 setup_isl68137.sh ${D}${localbindir}/setup_isl68137.sh
  install -m 755 power_monitor.py ${D}${localbindir}/power_monitor.py

  install -m 0755 rc.local ${D}/mnt/rc.local
  
  install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
  install -m 755 setup_module.sh ${D}${sysconfdir}/init.d/setup_module.sh
  update-rc.d -r ${D} setup_module.sh start 05 S .

  install -m 755 setup_i2c.sh ${D}${sysconfdir}/init.d/setup_i2c.sh
  update-rc.d -r ${D} setup_i2c.sh start 07 S .

  install -m 755 rsyslog_config.sh ${D}${sysconfdir}/init.d/rsyslog_config.sh
  update-rc.d -r ${D} rsyslog_config.sh start 61 S .

  #mount EMMC
  install -m 755 mount_emmc.sh ${D}${sysconfdir}/init.d/mount_emmc.sh
  update-rc.d -r ${D} mount_emmc.sh start 80 S .

  install -m 755 us_monitor.sh ${D}${localbindir}/us_monitor.sh
  install -m 755 start_us_monitor.sh ${D}${sysconfdir}/init.d/start_us_monitor.sh
  update-rc.d -r ${D} start_us_monitor.sh start 84 S .

  install -m 755 power-on.sh ${D}${sysconfdir}/init.d/power-on.sh
  update-rc.d -r ${D} power-on.sh start 85 S .

  install -m 755 setup_sensors.sh ${D}${sysconfdir}/init.d/setup_sensors.sh
  install -m 755 setup_platform.sh ${D}${sysconfdir}/init.d/setup_platform.sh
  update-rc.d -r ${D} setup_platform.sh start 100 2 3 4 5 .

  install -m 0755 fru-util ${D}${localbindir}/fru-util
  install -m 755 come_power.sh ${D}${localbindir}/come_power.sh
  install -m 755 version_dump ${D}${localbindir}/version_dump
  install -m 755 led_location.sh ${D}${localbindir}/led_location.sh
  install -m 755 power ${D}${localbindir}/power
  install -m 755 led_location ${D}${localbindir}/led_location
}

FILES_${PN} += "/usr/local ${sysconfdir} /mnt"

RDEPENDS_${PN} = "bash"
