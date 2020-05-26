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

if /usr/local/bin/boot_info.sh |grep "Slave Flash" ; then
    echo "BMC boot from Slave flash"
    logger "BMC boot from Slave flash"
    sys_led yellow on #sysled light yellow
else
    echo "BMC boot from Master flash"
    logger "BMC boot from Master flash"
    sys_led green on #sysled light green
fi

if /usr/local/bin/wedge_power.sh status |grep "on"; then
    echo "COMe is power on already"
else
    echo "COMe is power off, power on"
    logger "COMe is power off, power on"
    /usr/local/bin/wedge_power.sh on
fi

#Set BMC login timeout
echo "TMOUT=300" >> /etc/profile
echo "export TMOUT" >> /etc/profile

