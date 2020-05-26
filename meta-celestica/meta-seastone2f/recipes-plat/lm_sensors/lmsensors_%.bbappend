
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fishbone.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fishbone.conf ${D}${sysconfdir}/sensors.d/fishbone.conf
}
