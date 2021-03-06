
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fishbone.conf \
			file://phalanx.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fishbone.conf ${D}${sysconfdir}/sensors.d/fishbone.conf
    install -m 644 ../phalanx.conf ${D}${sysconfdir}/sensors.d/phalanx.conf
}
