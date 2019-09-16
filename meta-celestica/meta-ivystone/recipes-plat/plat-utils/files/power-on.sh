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

slave=0
if /usr/local/bin/boot_info.sh |grep "Slave Flash" ; then
    echo "BMC boot from Slave flash"
    logger "BMC boot from Slave flash"
    sys_led yellow on #sysled light yellow
    slave=1
else
    echo "BMC boot from Master flash"
    logger "BMC boot from Master flash"
    sys_led green on #sysled light green
    slave=0
fi

if [ $slave -eq 0 ]; then #master
    if [ -f /var/log/boot_slave ]; then
        ((val=$(cat /var/log/boot_slave)))
        if [ $val -eq 1 ]; then
            echo 0 > /var/log/boot_slave
            echo "Previous BMC booting from slave, switch to BMC slave flash booting"
            logger -p user.warning "Previous BMC booting from slave, switch to BMC slave flash booting"
            boot_from slave
        else
            echo "BMC switch to slave flash booting fail"
            logger -p user.warning "BMC switch to slave flash booting fail"
        fi
    fi
else #slave
    if [ -f /var/log/boot_slave ]; then
        echo 1 > /var/log/boot_slave
    fi
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

