# Copyright 2018-present Facebook. All Rights Reserved.

inherit aspeed_uboot_image

require recipes-core/images/fb-openbmc-image.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
IMAGE_FSTYPES += "cpio.lzma.u-boot"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  at93cx6-util \
  bcm5396-util \
  bitbang \
  cpldupdate \
  libcpldupdate-dll-gpio \
  flashrom \
  libpal \
  ipmid \
  lldp-util \
  mterm \
  memtester  \
  openbmc-utils \
  spatula \
  stress  \
  watchdog-ctrl \
  ncsi-util \
  rest-api \
  openbmc-gpio \
  plat-utils \
  tlv-eeprom \
  eeprom-data \
  fan-ctrl \
  peci-util \
  e2fsprogs \
  fruid \
  eeprom-bin \
  ipmbd \
  ipmitool \
  ipmb-util \
  crashdump \
  me-util \
  ethtool \
  mdio-util \
  usb-console \
  tcpdump \
  python3-pexpect \
  "

IMAGE_FEATURES += " \
  ssh-server-openssh \
  tools-debug \
  "

DISTRO_FEATURES += " \
  ext2 \
  ipv6 \
  nfs \
  usbgadget \
  usbhost \
  "

