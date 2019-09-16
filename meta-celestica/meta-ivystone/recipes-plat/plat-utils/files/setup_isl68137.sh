#!/bin/bash
#
# Copyright 2018-present Celestica. All Rights Reserved.
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

. /usr/local/bin/openbmc-utils.sh
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

((MAX_OUTPUT = 930))
((MIN_OUTPUT = 720))
((MAX_VALUE = 0xB2))
((MIN_VALUE = 0x02))

((OUTPUT_BASE = 50000))
((BASE_VALUE = $MAX_VALUE))
((DELTA_OUTPUT = 625))

FPGA_BUS=5
FPGA_ADDR=0x36
AVS_ADDR=0x30
((last_output = 0))
while true; do
    i2cset -f -y $FPGA_BUS $FPGA_ADDR 0x0 $AVS_ADDR
    ((value = $(i2cget -f -y $FPGA_BUS $FPGA_ADDR 0x0) ))
    # ((value = (value & 0xff00) >> 8))
    if [ $value -gt $MAX_VALUE ]; then
        ((value=$MAX_VALUE))
    elif [ $value -lt $MIN_VALUE ]; then
        ((value=$MIN_VALUE))
    fi

    ((output = ($BASE_VALUE - $value) * $DELTA_OUTPUT + $OUTPUT_BASE))
    ((output_voltage = $output/100 ))
    if [ $output_voltage -gt $MAX_OUTPUT ]; then
        ((output_voltage=$MAX_OUTPUT))
    elif [ $output_voltage -lt $MIN_OUTPUT ]; then
        ((output_voltage=$MIN_OUTPUT))
    fi

    if [ $output_voltage -ne $last_output ]; then
        logger "Set AVS Voltage: $output_voltage mv"
        ((last_output = $output_voltage))
        i2cset -f -y 17 0x60 0x21 $output_voltage w
    fi

    sleep 60
done
