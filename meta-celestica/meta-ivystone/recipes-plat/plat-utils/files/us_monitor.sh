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

FANS=4
PSU_NUM=2
psu_status=(0 0)
psu_path=(
"/sys/bus/i2c/devices/i2c-25/25-0059/hwmon"
"/sys/bus/i2c/devices/i2c-24/24-0058/hwmon"
)
psu_register=(
"i2c_device_delete 25 0x59;i2c_device_delete 25 0x51;i2c_device_add 25 0x59 dps1100;i2c_device_add 25 0x51 24c32;set_hwmon_threshold 25 59 in1_min 90000;set_hwmon_threshold 25 59 in1_max 264000"
"i2c_device_delete 24 0x58;i2c_device_delete 24 0x50;i2c_device_add 24 0x58 dps1100;i2c_device_add 24 0x50 24c32;set_hwmon_threshold 24 58 in1_min 90000;set_hwmon_threshold 24 58 in1_max 264000"
)

IVYSTONE_PSU_NUM=4
Ivystone_psu_status=(0 0 0 0)
Ivystone_psu_register=(
"i2c_device_delete 27 0x58;i2c_device_delete 27 0x50;i2c_device_add 27 0x58 dps1100;i2c_device_add 27 0x50 24c32;set_hwmon_threshold 27 58 in1_min 90000;set_hwmon_threshold 27 58 in1_max 264000"
"i2c_device_delete 26 0x58;i2c_device_delete 26 0x50;i2c_device_add 26 0x58 dps1100;i2c_device_add 26 0x50 24c32;set_hwmon_threshold 26 58 in1_min 90000;set_hwmon_threshold 26 58 in1_max 264000"
"i2c_device_delete 25 0x58;i2c_device_delete 25 0x50;i2c_device_add 25 0x58 dps1100;i2c_device_add 25 0x50 24c32;set_hwmon_threshold 25 58 in1_min 90000;set_hwmon_threshold 25 58 in1_max 264000"
"i2c_device_delete 24 0x58;i2c_device_delete 24 0x50;i2c_device_add 24 0x58 dps1100;i2c_device_add 24 0x50 24c32;set_hwmon_threshold 24 58 in1_min 90000;set_hwmon_threshold 24 58 in1_max 264000"
)

board_type=$(board_type)

psu_status_init() {
	id=0
	for i in "${psu_path[@]}"
	do
		if [ -e "$i" ]; then
			psu_status[$id]=1
		else
			logger "Warning: PSU $(($id + 1)) is absent"
			psu_status[$id]=0
		fi
		id=$(($id + 1))
	done
}

read_info() {
    echo `cat /sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/${3} | head -n 1`
}

psu_present() {
    if [ $1 -eq 1  ]; then
        ((val=$(read_info 0 0d psu_r_present)))
    else
        ((val=$(read_info 0 0d psu_l_present)))
    fi
    if [ $val -eq 0 ]; then
        return 1
    else
        return 0
    fi
}

psu_power() {
    if [ $1 -eq 1  ]; then
        ((val=$(read_info 0 0d psu_r_status)))
    else
        ((val=$(read_info 0 0d psu_l_status)))
    fi
    if [ $val -eq 0 ]; then
        return 1
    else
        return 0
    fi
}


psu_status_check() {
	if [ $# -le 0 ]; then
		return 1
	fi

	psu_present $(($1 + 1))
	if [ $? -eq 1 ]; then
		psu_power $(($1 + 1))
		power_ok=$?
		if [ ${psu_status[$1]} -eq 0 ] && [ $power_ok -eq 0 ]; then
			logger "us_monitor: Register PSU $(($i + 1))"
			eval "${psu_register[$1]}"
			psu_status_init
			return 0
		fi
	fi
}


Ivystone_psu_present() {
    ((psu_num=$IVYSTONE_PSU_NUM - $1 + 1))
    ((val=$(read_info 0 0d psu_${psu_num}_present)))
    if [ $val -eq 0 ]; then
        return 1
    else
        return 0
    fi
}

Ivystone_psu_power() {
    ((psu_num=$IVYSTONE_PSU_NUM - $1 + 1))
    ((val=$(read_info 0 0d psu_${psu_num}_status)))
    if [ $val -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

Ivystone_psu_status_init() {
    for((i = 0; i < $IVYSTONE_PSU_NUM; i++))
    do
	    Ivystone_psu_present $(($i + 1))
	    if [ $? -eq 1 ]; then
		    Ivystone_psu_power $(($i + 1))
		    power_ok=$?
		    if [ $power_ok -eq 1 ]; then
			    Ivystone_psu_status[$i]=1
		    fi
	    fi
    done
}
Ivystone_psu_status_check() {
	if [ $# -le 0 ]; then
		return 1
	fi

	Ivystone_psu_present $(($1 + 1))
	if [ $? -eq 1 ]; then
		Ivystone_psu_power $(($1 + 1))
		power_ok=$?
		if [ ${Ivystone_psu_status[$1]} -eq 0 ] && [ $power_ok -eq 1 ]; then
			logger "us_monitor: Register PSU $(($i + 1))"
			eval "${Ivystone_psu_register[$1]}"
			Ivystone_psu_status[$1]=1
			return 0
		fi
	fi
}

get_fan_pwm() {
    for i in $FANS ; do
        pwm_node="${FANCPLD_SYSFS_DIR}/fan${i}_pwm"
        val=$(cat $pwm_node | head -n 1)
        if [ $((val * 100 % 255)) -ne 0 ]; then
            pwm=$((val * 100 / 255 + 1))
        else
            pwm=$((val * 100 / 255))
        fi
        if [ $pwm -gt 0 ]; then
            return $pwm
        fi
    done

    return 0
}

get_fan_dir() {
    for i in $FANS ; do
        val=$(/usr/local/bin/fruid-util fan$i |grep R1241-F9001)
        if [ -n "$val" ]; then
            echo "F2B"
            return 1
        fi
    done
    echo "B2F"
}

cpu_temp_update() {
    if /usr/local/bin/wedge_power.sh status |grep "off"; then
        echo 0 >/sys/bus/i2c/devices/i2c-0/0-000d/temp2_input
        return 0
    fi
    if /usr/local/bin/wedge_power.sh status |grep "on"; then
        temp=$(get_cpu_temp)
        if [ -z "$temp" ]; then
            return 0
        fi
        val=$(($temp*1000))
        echo $val >/sys/bus/i2c/devices/i2c-0/0-000d/temp2_input
    fi
}

fan_wdt_monitor() {
    if [ $# -lt 1 ]; then
        return 0
    fi
    ((val=$(cat $FAN_WDT_STATUS 2> /dev/null | head -n 1)))
    if [ -z "$val" ]; then
        return $1
    elif [ $val -eq 1 ]; then
        if [ $1 -eq 0 ]; then
            logger -p user.error "FAN watchdog timeout, FAN speed is set to 100%"
        fi
        return 1            #fan wdt assert
    elif [ $val -eq 0 ]; then
        if [ $1 -eq 1 ]; then
            logger -p user.warning "FAN watchdog recovered"
        fi
        return 0
    fi

}

come_status_monitor() {
    if [ $# -lt 1 ]; then
        return 0
    fi
    ((val=$(cat $USRV_STATUS_SYSFS 2> /dev/null | head -n 1)))
	if [ -z "$val" ]; then
        return 0
    fi
    if [ $val -ne $1 ]; then
        ((st=$val&0x8))
        if [ $st -gt 0 ]; then
            logger -p user.crit "CPU state changed to power saving mode SUS_STAT"
        fi
        ((st=$val&0x4))
        if [ $st -gt 0 ]; then
            logger -p user.crit "CPU state changed to power saving mode S5"
        fi
        ((st=$val&0x2))
        if [ $st -gt 0 ]; then
            logger -p user.crit "CPU state changed to power saving mode S4"
        fi
        ((st=$val&0x1))
        if [ $st -gt 0 ]; then
            logger -p user.crit "CPU state changed to power saving mode S3"
        fi
        ((tmp=$(i2cget -f -y 0 0x0d 0x18 2> /dev/null | head -n 1)))
        ((st=$tmp&0x08))
        if [ $st -gt 0 ]; then
            logger -p user.warning "CPU state changed to S0"
        fi
    fi
    return $val
}

bios_boot_monitor() {
    if [ $# -lt 1 ]; then
        return 0
    fi
    ((boot_source=$(cat $BIOS_BOOT_CHIP | head -n 1)))
    ((boot_status=$(cat $BIOS_BOOT_STATUS | head -n 1)))

    if [ $boot_source -eq 1 ]; then #boot from slave
        if [ $boot_status -eq 1 ]; then
            if [ $1 -ne 1 ]; then
                logger -p user.warning "BIOS boot from secondary flash succeed"
                sys_led yellow on
            fi
            return 1
        else
            if [ $1 -ne 2 -a $1 -ge 72 ]; then
                logger -p user.crit "BIOS boot from secondary flash failed"
                sys_led yellow slow
                return 2
            elif [ $1 -ne 2 ]; then
                return $(($1+3))
            fi
            return 2
        fi
    else
    	if [ $boot_status -eq 1 ]; then
            if [ $1 -ne 1 ]; then
                logger -p user.notice "BIOS boot from primary flash succeed"
                sys_led green on
            fi
            return 1
        else
            if [ $1 -ne 2 -a $1 -ge 72 ]; then
                logger -p user.error "BIOS boot from primary flash failed"
                return 2
            elif [ $1 -ne 2 ]; then
                return $(($1+3))
            fi
            return 2
        fi
    fi

}

come_aer_err_monitor() {
    if [ $# -lt 1 ]; then
        return 0;
    fi
    ret=0
    ((val=$(gpio_get B6)))
    if [ $val -eq 0 ]; then
        ((temp=$1&0x1))
        if [ $temp -eq 0 ]; then
            logger "ERR[0] is asserted"
        fi
        ret=$(($ret+1))
    fi
    ((val=$(gpio_get B7)))
    if [ $val -eq 0 ]; then
        ((temp=$1&0x2))
        if [ $temp -eq 0 ]; then
            logger "ERR[1] is asserted"
        fi
        ret=$(($ret+2))
    fi
    ((val=$(gpio_get AA0)))
    if [ $val -eq 0 ]; then
        ((temp=$1&0x4))
        if [ $temp -eq 0 ]; then
            logger "ERR[2] is asserted"
        fi
        ret=$(($ret+4))
    fi

    return $ret
}

come_mca_err_monitor() {
    count=0
    change=0
    temp=1
    val=1
    if [ $# -lt 1 ]; then
        return 0
    fi
    ((temp=$(gpio_get B5)))
    while [ $count -lt 16 ];
    do
        ((val=$(gpio_get B5)))
        if [ $val -ne $temp ]; then
            change=$(($change+1))
            temp=val
        fi
        count=$(($count+1))
    done
    if [ $change -ge 2 -a $1 -ne 1 ]; then
        logger "CATERR# is asserted for 16 BCLKs"
        return 1
    fi

    if [ $val -eq 0 -a $1 -ne 2 ]; then
        logger "CATERR# remains asserted"
        return 2
    fi

    if [ $val -eq 1 -a $1 -ne 0 ]; then
        logger "CATERR# recovers asserted"
        return 0
    fi

    return $1
}

come_wdt_monitor() {
    if [ -f "/tmp/watchdog" ]; then
        ((val=$(cat /tmp/watchdog)))
        if [ $val -eq 0 ]; then
            logger -p user.warning "Host CPU watchdog disabled"
            come_wdt_enable=0
            come_wdt_count=0
            rm /tmp/watchdog
            return 0
        elif [ $come_wdt_count -eq 0 ]; then
            if [ $come_wdt_enable -eq 0 ]; then
                logger -p user.warning "Host CPU watchdog enabled"
            fi
            come_wdt_enable=1
            come_wdt_count=$(($val/7+1))
            rm /tmp/watchdog
        fi
    fi

    if [ $come_wdt_enable -eq 1 ]; then
        if [ $come_wdt_count -gt 0 ]; then
            come_wdt_count=$(($come_wdt_count-1))
        else
            logger -p user.crit "Host CPU watchdog timeout"
            come_wdt_enable=0
        fi
    fi
}

rsyslog_update() {
    pid=$(ps |grep rsyslogd |grep -v grep | awk -F ' ' '{print $1}')
    if [ ! -n "$pid" ]; then
        logger "The rsyslogd can not be found, restart it"
        /etc/init.d/syslog.rsyslog restart
    fi
}

cpld_refresh_monitor() {
    if [ -f "/tmp/cpld_refresh" ]; then
        para=$(cat /tmp/cpld_refresh)
        cpld_refresh $para
        rm /tmp/cpld_refresh
    fi
}

cpu_error_autodump() {
    val=$(cat /sys/class/misc/cpu_error/error)
    if [ "$val" = "1" ]; then
        logger -p user.warning "CPU error detected, auto dump it"
        /usr/local/bin/autodump.sh &
        echo 0 >/sys/class/misc/cpu_error/error
    fi
}

Ivystone_psu_status_init

come_rest_status 2
come_rst_st=$?
cpu_update=0
fan_wdt_st=0
come_val=0
bios_status=0
#aer_error=0
#mca_error=0
come_wdt_count=0
come_wdt_enable=0
bmc_boot_check=20

echo 70000 >/sys/bus/i2c/devices/i2c-7/7-004d/hwmon/hwmon3/temp1_max
echo 60000 >/sys/bus/i2c/devices/i2c-7/7-004d/hwmon/hwmon3/temp1_max_hyst



while true; do
    for((i = 0; i < $IVYSTONE_PSU_NUM; i++))
    do
	    Ivystone_psu_status_check $i
    done

	come_rest_status 1
	if [ $? -ne $come_rst_st ]; then
		come_rest_status 2
		come_rst_st=$?
	fi

    cpu_update=$((cpu_update+1))
    if [ $cpu_update -ge 6 ]; then
        cpu_temp_update
        cpu_update=0
    fi

    #monitor fan WDT
    fan_wdt_monitor $fan_wdt_st
    fan_wdt_st=$?

    #monitor COME status
    come_status_monitor $come_val
    come_val=$?

    #BIOS boot monitor
    bios_boot_monitor $bios_status
    bios_status=$?

    #COMe AER error monitor
    #come_aer_err_monitor $aer_error
    #aer_error=$?

    #COMe MCA error monitor
    #come_mca_err_monitor $mca_error
    #mca_error=$?

    #COMe hang watchdog monitor
    come_wdt_monitor

    rsyslog_update

    cpld_refresh_monitor

    if [ $bmc_boot_check -gt 1 ]; then
        bmc_boot_check=$(($bmc_boot_check-1))
    elif [ $bmc_boot_check -eq 1 ]; then
        bmc_boot_check=0
        if /usr/local/bin/boot_info.sh |grep "Slave Flash" ; then
            logger -p user.warning "BMC boot from slave flash succeded"
        else
            logger -p user.warning "BMC boot from master flash succeded"
        fi
    fi

    cpu_error_autodump

    usleep 3000000
done
