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
#

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

prog="$0"

usage() {
    echo "Usage: $prog <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current microserver power status"
    echo
    echo "  on: Power on microserver if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if microserver has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off microserver ungracefully"
    echo
    echo "  reset: Power reset microserver ungracefully"
    echo
    echo "  cycle: Power cycle microserver ungracefully"
    echo
}

do_status() {
    echo -n "Microserver power is "
	wedge_is_us_on
	ret=$?
    if [ $ret -eq 0 ]; then
        echo "on"
    elif [ $ret -eq 1 ]; then
        echo "off"
	else
		echo "fail, please power cycle it!"
        logger "Micro-server power status Error($ret)!"
    fi
    return 0
}

do_on() {
    local force opt ret
    force=0
    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit -1
                ;;

        esac
    done
    echo -n "Power on microserver ..."
    if [ $force -eq 0 ]; then
        # need to check if uS is on or not
        if wedge_is_us_on 10 "."; then
            echo " Already on. Skip!"
            return 1
        fi
    fi

    # power on sequence
    wedge_power on
    ret=$?
    if [ $ret -eq 0 ]; then
        echo " Done"
        logger "Successfully power on micro-server"
    else
        echo " Failed"
        logger "Failed to power on micro-server"
    fi
    return $ret
}

do_off() {
    local ret
    echo -n "Power off microserver(5s) ..."
    wedge_power off
    ret=$?
    if [ $ret -eq 1 ]; then
        echo " Done"
        logger "Successfully power off micro-server"
    else
        echo " Failed"
        logger "Failed to power off micro-server"
    fi
    return $ret
}

do_reset() {
    if ! wedge_is_us_on; then
        echo "Power resetting microserver that is powered off has no effect."
        echo "Use '$prog on' to power the microserver on"
        return 1
    fi
    echo -n "Power reset microserver(10s) ..."
	wedge_power reset
    ret=$?
    if [ $ret -eq 0 ]; then
        echo " Done"
        logger "Successfully power reset micro-server"
    else
        echo " Failed"
        logger "Failed to power reset micro-server"
    fi
    return 0
}

do_cycle() {
    echo 0 > /sys/bus/i2c/devices/0-000d/pwr_cycle
    return $?
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

command="$1"
shift

case "$command" in
    status)
        do_status $@
        ;;
    on)
        do_on $@
        ;;
    off)
        do_off $@
        ;;
    reset)
        do_reset $@
        ;;
    cycle)
        do_cycle
        ;;
    *)
        usage
        exit -1
        ;;
esac

exit $?
