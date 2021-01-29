# Copyright 2015-present Facebook. All rights reserved.
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

from openbmc_gpio_table import BoardGPIO

board_gpio_table_v1 = [
    BoardGPIO('GPIOB0', 'CB_SUS_STAT_N'),
    BoardGPIO('GPIOB1', 'CB_SUS_S4_N'),
    BoardGPIO('GPIOB2', 'CB_RST_N'),
    BoardGPIO('GPIOB3', 'CB_WDT'),
    BoardGPIO('GPIOB4', 'CB_SYS_RST_N'),
    BoardGPIO('GPIOB5', 'CATERR_N'),
    BoardGPIO('GPIOB6', 'CPU_ERROR_0_N'),
    BoardGPIO('GPIOB7', 'CPU_ERROR_1_N'),
    BoardGPIO('GPIOF2', 'BMC_EEPROM_SEL'),
    BoardGPIO('GPIOF3', 'BMC_EEPROM_SS'),
    BoardGPIO('GPIOF4', 'BMC_EEPROM_MISO'),
    BoardGPIO('GPIOF5', 'BMC_EEPROM_MOSI'),
    BoardGPIO('GPIOF6', 'BMC_EEPROM_SCK'),
    BoardGPIO('GPIOF7', 'BMC_INT_IN'),
    BoardGPIO('GPIOAA0', 'CPU_ERROR_2_N'),
    BoardGPIO('GPIOAA1', 'BMC_SWITCH_RST_OUT_N'),
    BoardGPIO('GPIOAA4', 'BMC_JTAG_TDO'),
    BoardGPIO('GPIOAA5', 'BMC_JTAG_TDI'),
    BoardGPIO('GPIOAA6', 'BMC_JTAG_TMS'),
    BoardGPIO('GPIOAA7', 'BMC_JTAG_TCK'),
    BoardGPIO('GPIOAB0', 'EMMC_RST_N'),
    BoardGPIO('GPIOE0', 'BMC_SPI_WP0_N'),
    BoardGPIO('GPIOE1', 'BMC_SPI_WP1_N'),
    BoardGPIO('GPIOE4', 'BIOS_UPGRADE_CPLD'),
    BoardGPIO('GPIOE5', 'BIOS_MUX_SWITCH'),
    BoardGPIO('GPIOE6', 'BMC_GPIOD6'),
    BoardGPIO('GPIOE7', 'BMC_GPIOD7'),
    BoardGPIO('GPIOG4', 'BMC_I2C_ALT1'),
    BoardGPIO('GPIOG5', 'BMC_I2C_ALT2'),
    BoardGPIO('GPIOG6', 'BMC_I2C_ALT3'),
    BoardGPIO('GPIOG7', 'BMC_I2C_ALT4'),
    BoardGPIO('GPIOH5', 'MDC_MDIO_SELECT'),
    BoardGPIO('GPIOJ1', 'BMC_RST_OUT'),
    BoardGPIO('GPIOL0', 'RSVD_NCTS1'),
    BoardGPIO('GPIOL1', 'RSVD_NDCD1'),
    BoardGPIO('GPIOL2', 'BMC_GPIO0'),
    BoardGPIO('GPIOL3', 'RSVD_NRI1'),
    BoardGPIO('GPIOL4', 'RSVD_NDTR1'),
    BoardGPIO('GPIOL5', 'RSVD_NRTS1'),
    BoardGPIO('GPIOM4', 'BMC_FPGA_GPIO1'),
    BoardGPIO('GPIOM5', 'BMC_FPGA_GPIO2'),
    BoardGPIO('GPIOS1', 'BMC_INT_OUT'),
    BoardGPIO('GPIOS2', 'COME_12V_EN_B'),
    BoardGPIO('GPIOS3', 'COME_5V_EN_BMC'),
    BoardGPIO('GPIOY0', 'CB_SUS_S3_N'),
    BoardGPIO('GPIOY1', 'CB_SUS_S5_N'),
    BoardGPIO('GPIOY2', 'SIOPWREQ'),
    BoardGPIO('GPIOY3', 'SIOONCTRL'),
    BoardGPIO('GPIOZ0', 'RESET_BTN_N'),
    BoardGPIO('GPIOZ1', 'CB_PWR_OK'),
]
