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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
      file://disable_watchdog.sh \
      file://boot_info.sh \
      file://create_vlan_intf \
      file://config_bmc_mac.sh \
      "

RDEPENDS_${PN} += " python3 bash"
DEPENDS_append += " update-rc.d-native"

do_install_append() {
    localbindir="/usr/local/bin"
	pkgdir="/usr/local/fbpackages/utils"
    install -d ${D}${localbindir}
	install -d ${D}${pkgdir}
	ln -s "/usr/local/bin/openbmc-utils.sh" "${D}${pkgdir}/ast-functions"


    install -m 0755 ${WORKDIR}/disable_watchdog.sh ${D}${sysconfdir}/init.d/disable_watchdog.sh
    update-rc.d -r ${D} disable_watchdog.sh start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/config_bmc_mac.sh ${D}${sysconfdir}/init.d/config_bmc_mac.sh
    update-rc.d -r ${D} config_bmc_mac.sh start 70 5 .

	install -m 0755 boot_info.sh ${D}${localbindir}/boot_info.sh

	# create VLAN intf automatically
    install -d ${D}/${sysconfdir}/network/if-up.d
    install -m 755 create_vlan_intf ${D}${sysconfdir}/network/if-up.d/create_vlan_intf
}

FILES_${PN} += "${sysconfdir}"
