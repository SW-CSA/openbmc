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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
. /usr/local/bin/openbmc-utils.sh


if [ $# -ge 1 ]; then
    if [ "$1" = "on" ]; then
         i2cset -f -y 5 0x36 0x0 0xbc24 w
         i2cset -f -y 5 0x36 0x0 0x0140 w
         i2cset -f -y 5 0x36 0x0 0x0024 w
    elif [ "$1" = "off" ]; then
         i2cset -f -y 5 0x36 0x0 0xbc24 w
         i2cset -f -y 5 0x36 0x0 0x1340 w
         i2cset -f -y 5 0x36 0x0 0x0024 w
    fi
else
    ((val=$(i2cset -f -y 5 0x36 0x0 0x40;i2cget -f -y 5 0x36 | head -n 1)))
    ((ret=$val&0x10))
    if [ $ret -gt 0 ]; then
        echo "The LED location is off"
    else
        ((ret=$val&0x2))
        if [ $ret -eq 0 ]; then
            ((ret=$val&0x4))
            if [ $ret -eq 0 ]; then
                echo "The LED location is on"
            fi
        else
            echo "The LED location is off"
        fi
    fi
fi
