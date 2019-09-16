#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
. /usr/local/bin/openbmc-utils.sh

#For Common I2C devices
# Bus 0
i2c_device_add 0 0x0d syscpld  #CPLD

# Bus 1
i2c_device_add 1 0x50 24c32  #CPU EEPROM

# Bus 2
i2c_device_add 2 0x51 24c64  #Switch EEPROM
i2c_device_add 2 0x53 24c64  #BMC EEPROM
i2c_device_add 2 0x57 24c64  #SYS EEPROM
# Bus 2 for i2c-2 PCA9548
i2c_device_add 2 0x77 pca9548
#bus 9 channel 0 For LC EEPROM
i2c_device_add 9 0x52 24c64
#bus 10 channel 1 For LC EEPROM
i2c_device_add 10 0x52 24c64
#bus 11 channel 2
#bus 12 channel 3
#bus 14 channel 4
#bus 15 channel 5
#bus 40 channel 6
#bus 41 channel 7

# Bus 3

# Bus 4
i2c_device_add 4 0x15 ir3584  #IR3584 in COMe
i2c_device_add 4 0x16 ir3584  #IR3584 in COMe
i2c_device_add 4 0x35 max11617 #Max11617 ADC
i2c_device_add 4 0x42 ir38062  #IR38062
# Bus 16 for i2c-4 PCA9548
#bus 16 channel 0
i2c_device_add 16 0x70 ir3584  #IR3584
i2c_device_add 16 0x49 ir38062  #IR38062
#bus 17 channel 1
i2c_device_add 17 0x45 ir38060  #IR38060
i2c_device_add 17 0x49 ir38062  #IR38062
i2c_device_add 17 0x60 isl68137  #ISL68137
#bus 18 channel 2
#bus 19 channel 3
i2c_device_add 19 0x30 ir3584  #IR3584
i2c_device_add 19 0x50 ir3584  #IR3584
i2c_device_add 19 0x70 ir3584  #IR3584
#bus 20 channel 4
i2c_device_add 20 0x50 ir3584  #IR3584
i2c_device_add 20 0x70 ir3584  #IR3584
i2c_device_add 20 0x45 ir38060  #IR38060
#bus 21 channel 5
i2c_device_add 21 0x30 ir3584  #IR3584
i2c_device_add 21 0x50 ir3584  #IR3584
i2c_device_add 21 0x70 ir3584  #IR3584
#bus 22 channel 6
i2c_device_add 22 0x50 ir3584  #IR3584
i2c_device_add 22 0x70 ir3584  #IR3584
i2c_device_add 22 0x45 ir38060  #IR38060
#bus 23 channel 7
i2c_device_add 23 0x45 ir38060  #IR38060

# Bus 5
#i2c_device_add 5 0x36 fpga    #FPGA

# Bus 6
# Bus 26 for i2c-6 PCA9548
#bus 24 channel 0
i2c_device_add 24 0x50 24c32 #PSU4 FRU EEPROM
i2c_device_add 24 0x58 dps1100 #PSU4 PMBUS
#bus 25 channel 1
i2c_device_add 25 0x50 24c32 #PSU3 FRU EEPROM
i2c_device_add 25 0x58 dps1100 #PSU3 PMBUS
#bus 26 channel 2
i2c_device_add 26 0x50 24c32 #PSU2 FRU EEPROM
i2c_device_add 26 0x58 dps1100 #PSU2 PMBUS
#bus 27 channel 3
i2c_device_add 27 0x50 24c32 #PSU1 FRU EEPROM
i2c_device_add 27 0x58 dps1100 #PSU1 PMBUS
#bus 28 channel 4
#bus 29 channel 5
#bus 30 channel 6
i2c_device_add 30 0x20 pca9505
#bus 31 channel 7
i2c_device_add 31 0x48 tmp75
i2c_device_add 31 0x49 tmp75

# # Bus 7
i2c_device_add 7 0x4C max31730
i2c_device_add 7 0x4d tmp75
i2c_device_add 7 0x4E tmp75
i2c_device_add 7 0x4F tmp75
# Bus 7 for i2c-7 PCA9548
i2c_device_add 7 0x77 pca9548
#bus 42 channel 0 For Temp
i2c_device_add 42 0x48 tmp75
i2c_device_add 42 0x49 tmp75
#bus 43 channel 1 For Temp
i2c_device_add 43 0x48 tmp75
i2c_device_add 43 0x49 tmp75
#bus 44 channel 2
#bus 45 channel 3
#bus 46 channel 4
#bus 47 channel 5
#bus 48 channel 6
#bus 49 channel 7

# Bus 8
i2c_device_add 8 0x0d fancpld #Fan CPLD
# Bus 32 for i2c-8 PCA9548
#bus 32 channel 0
i2c_device_add 32 0x50 24c64 #Fan EEPROM
#bus 33 channel 1
i2c_device_add 33 0x50 24c64 #Fan EEPROM
#bus 34 channel 2
i2c_device_add 34 0x50 24c64 #Fan EEPROM
#bus 35 channel 3
i2c_device_add 35 0x50 24c64 #Fan EEPROM
#bus 36 channel 4
i2c_device_add 36 0x50 24c64 #Fan EEPROM

#bus 39 channel 7
i2c_device_add 39 0x56 24c64 #Fan Board EEPROM
i2c_device_add 39 0x48 tmp75
i2c_device_add 39 0x49 tmp75

#isl68l38 output voltage
/usr/local/bin/setup_isl68137.sh > /dev/null 2>&1 &
#Initialize PCA9506 GPIOs
setup_pca9506.sh

# run sensors.config set command
#sensors -s
