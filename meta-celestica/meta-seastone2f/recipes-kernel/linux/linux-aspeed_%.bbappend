LINUX_VERSION_EXTENSION = "-seastone2f"

COMPATIBLE_MACHINE = "seastone2f"

KERNEL_MODULE_AUTOLOAD += " \
    ast_adc \
    pmbus_core \
    pfe3000 \
"

# KERNEL_MODULE_PROBECONF += "                    \
#  i2c-mux-pca954x                                \
# "

# module_conf_i2c-mux-pca954x = "options i2c-mux-pca954x ignore_probe=1"


FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
			file://0001-Add-Celestica-Questone2-and-AS58XX-CL-projects.patch \
           "
