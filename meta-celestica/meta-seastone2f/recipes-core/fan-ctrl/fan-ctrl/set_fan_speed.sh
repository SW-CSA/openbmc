#!/bin/sh
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

board_type=$(board_type)
if [ "$board_type" = "Phalanx" ]; then
    FAN_TOTAL=5
else
    FAN_TOTAL=4
fi

usage() {
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (1..$FAN_TOTAL)> " >&2
}

FAN_DIR=/sys/bus/i2c/devices/i2c-8/8-000d/

set -e

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

if [ "$#" -eq 1 ]; then
	if [ "$board_type" = "Phalanx" ]; then
		FANS="1 2 3 4 5"
	else
    	FANS="1 2 3 4"
	fi
else
    if [ $2 -gt $FAN_TOTAL ]; then
        usage
        exit 1
    fi
    FANS="$2"
fi

# Convert the percentage to pwm
unit=$(( ( $1 * 255 ) / 100 ))

for fan in $FANS; do
    real_fan=$(($FAN_TOTAL-$fan+1))
    pwm="${FAN_DIR}/fan${real_fan}_pwm"
    echo "$unit" > $pwm
    echo "Successfully set fan ${fan} speed to $1%"
done
