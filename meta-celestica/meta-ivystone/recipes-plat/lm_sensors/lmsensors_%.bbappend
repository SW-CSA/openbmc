
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://Ivystone.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../Ivystone.conf ${D}${sysconfdir}/sensors.d/Ivystone.conf
}
