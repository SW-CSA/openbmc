
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://seastone2f.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../seastone2f.conf ${D}${sysconfdir}/sensors.d/seastone2f.conf
}
