#!/bin/bash

SYSCPLD_SYSFS_DIR="/sys/bus/i2c/devices/i2c-0/0-000d"
FANCPLD_SYSFS_DIR="/sys/bus/i2c/devices/i2c-8/8-000d"
HARDWARE_VERSION="${SYSCPLD_SYSFS_DIR}/hardware_version"
USRV_STATUS_SYSFS="${SYSCPLD_SYSFS_DIR}/come_status"
PWR_BTN_SYSFS="${SYSCPLD_SYSFS_DIR}/cb_pwr_btn_n"
PWR_RESET_SYSFS="${SYSCPLD_SYSFS_DIR}/come_rst_n"
SYSLED_CTRL_SYSFS="${SYSCPLD_SYSFS_DIR}/sysled_ctrl"
SYSLED_SEL_SYSFS="${SYSCPLD_SYSFS_DIR}/sysled_select"
SYSLED_SOL_CTRL_SYSFS="${SYSCPLD_SYSFS_DIR}/sol_control"
SYSLED_BCM5387_RST_SYSFS="${SYSCPLD_SYSFS_DIR}/bcm5387_reset"
BIOS_CHIPSELECTION="${SYSCPLD_SYSFS_DIR}/bios_cs"
BIOS_CTRL="${SYSCPLD_SYSFS_DIR}/bios_ctrl"
BIOS_BOOT_STATUS="${SYSCPLD_SYSFS_DIR}/bios_boot_ok"
BIOS_BOOT_CHIP="${SYSCPLD_SYSFS_DIR}/bios_boot_cs"
COME_RESET_STATUS_SYSFS="${SYSCPLD_SYSFS_DIR}/come_rst_st"
FAN_WDT_STATUS="${FANCPLD_SYSFS_DIR}/wdt_status"

wedge_power() {
	if [ "$1" == "on" ]; then
		echo 0 > $PWR_BTN_SYSFS
		sleep 1
		echo 1 > $PWR_BTN_SYSFS
	elif [ "$1" == "off" ]; then
		echo 0 > $PWR_BTN_SYSFS
		sleep 5
		echo 1 > $PWR_BTN_SYSFS
	elif [ "$1" == "reset" ]; then
		echo 0 > $PWR_RESET_SYSFS
		sleep 10
	elif [ "$1" == "cycle" ]; then
		echo 0 > /sys/bus/i2c/devices/0-000d/pwr_cycle
		sleep 25
	else
		echo -n "Invalid parameter"
		return 1
	fi
	wedge_is_us_on
	return $?
}

wedge_is_us_on() {
    local val n retries prog
    if [ $# -gt 0 ]; then
        retries="$1"
    else
        retries=1
    fi
    if [ $# -gt 1 ]; then
        prog="$2"
    else
        prog=""
    fi
    if [ $# -gt 2 ]; then
        default=$3              # value 0 means defaul is 'ON'
    else
        default=1
    fi
    ((val=$(cat $USRV_STATUS_SYSFS 2> /dev/null | head -n 1)))
	if [ -z "$val" ]; then
        return $default
	elif [ $val -eq 15 ]; then
        return 0            # powered on
    elif [ $val -eq 8 ]; then
        return 1
	else
		echo -n "read value $val "
		return 2
    fi
}

sys_led_usage() {
	echo "option: "
	echo "<green| yellow| mix| off> #LED color select"
	echo "<on| off| fast| slow>     #LED turn on, off or blink"
	echo 
}

sys_led_show() {
	local val
	sel=$(cat $SYSLED_SEL_SYSFS 2> /dev/null | head -n 1)
	ctrl=$(cat $SYSLED_CTRL_SYSFS 2> /dev/null | head -n 1)

	case "$sel" in
		0x0)
			val="green and yellow"
			;;
		0x1)
			val="green"
			;;
		0x2)
			val="yellow"
			;;
		0x3)
			val="off"
			;;
	esac
	echo -n "LED: $val "
	
	case "$ctrl" in
		0x0)
			val="on"
			;;
		0x1)
			val="1HZ blink"
			;;
		0x2)
			val="4HZ blink"
			;;
		0x3)
			val="off"
			;;
	esac
	echo "$val"
}

sys_led() {
	if [ $# -lt 2 ]; then
		sys_led_show
		return 0
	fi
	if [ "$1" == "green" ]; then
		echo 0x1 > $SYSLED_SEL_SYSFS
	elif [ "$1" == "yellow" ]; then
		echo 0x2 > $SYSLED_SEL_SYSFS
	elif [ "$1" == "mix" ]; then
		if [ "$2" == "on" ];then
			echo "Mode [mix on] not support!"
			return 1
		fi
		echo 0x0 > $SYSLED_SEL_SYSFS
	elif [ "$1" == "off" ]; then
		echo 0x3 > $SYSLED_SEL_SYSFS
	else
		sys_led_usage
		return 1
	fi

	if [ "$2" == "on" ]; then
		echo 0x0 > $SYSLED_CTRL_SYSFS
	elif [ "$2" == "fast" ]; then
		echo 0x2 > $SYSLED_CTRL_SYSFS
	elif [ "$2" == "slow" ]; then
		echo 0x1 > $SYSLED_CTRL_SYSFS
	elif [ "$2" == "off" ]; then
		echo 0x3 > $SYSLED_CTRL_SYSFS
	else
		sys_led_usage
		return 1
	fi
}

board_type() {
	((val=$(i2cget -f -y 0 0x0d 0x02 2> /dev/null | head -n 1)))
	if [ $val -eq '0' ]; then
	    ((val=$(i2cget -f -y 0 0x0d 0x03 2> /dev/null | head -n 1)))
	    if [ $val -eq '0' ]; then
		    echo "Fishbone32"
        else
		    echo "Fishbone48"
        fi
    else
        echo "Phalanx"
	fi
}

sol_ctrl() {
	if [ "$1" = "BMC" ]; then
		echo 0 > $SYSLED_SOL_CTRL_SYSFS
	elif [ "$1" = "COME" ]; then
		echo 1 > $SYSLED_SOL_CTRL_SYSFS
	fi
}

boot_from() {
	if [ $# -lt 1 ]; then
		echo ""
		echo "Please indicate master or slave"
		return 1
	fi

	boot_source=0x00000013
	wdt2=$(devmem 0x1e785030)
	((boot_code_source = (wdt2 & 0x2) >> 1))
	if [ "$1" = "master" ]; then
		if [ $boot_code_source = 0 ]; then
			echo "Current boot source is master, no need to switch."
			return 0
		fi
		boot_source=0x00000093
	elif [ "$1" = "slave" ]; then
		if [ $boot_code_source = 1 ]; then
			echo "Current boot source is slave, no need to switch."
			return 0
		fi
		boot_source=0x00000093
	else
		echo "Error parameter!"
		return 1
	fi
	devmem 0x1e785024 32 0x00989680
	devmem 0x1e785028 32 0x4755
	devmem 0x1e78502c 32 $boot_source
}

bios_upgrade() {
        source /usr/local/bin/openbmc-utils.sh
        gpio_set E4 1

        if [ ! -c /dev/spidev1.0 ]; then
                mknod /dev/spidev1.0 c 153 0
        fi
        modprobe spidev

        if [ "$1" == "master" ]; then
                echo 0x2 > $BIOS_CTRL
                echo 0x1 > $BIOS_CHIPSELECTION
        elif [ "$1" == "slave" ]; then
                echo 0x2 > $BIOS_CTRL
                echo 0x3 > $BIOS_CHIPSELECTION
        else
                echo "bios_upgrade [master/slave] [flash type] [operation:r/w/e] [file name]"
        fi

        if [ $# == 4 ]; then
                flashrom -p linux_spi:dev=/dev/spidev1.0 -c $2 -$3 $4
        elif [ $# == 3 ] && [ "$3" == "e" ]; then
                flashrom -p linux_spi:dev=/dev/spidev1.0 -c $2 -E
        elif [ $# == 2 ]; then
                flashrom -p linux_spi:dev=/dev/spidev1.0 -c $2
        fi

        gpio_set E4 0
        echo 0x1 > $BIOS_CTRL
        echo 0x1 > $BIOS_CHIPSELECTION
}

come_reset() {
        if [ "$1" == "master" ]; then
                echo 0x0 > $BIOS_CTRL
                echo 0x1 > $BIOS_CHIPSELECTION
                /usr/local/bin/wedge_power.sh cycle
        elif [ "$1" == "slave" ]; then
                echo 0x0 > $BIOS_CTRL
                echo 0x3 > $BIOS_CHIPSELECTION
                /usr/local/bin/wedge_power.sh cycle
        else
                echo "come_reset [master/slave]"
        fi
}

come_boot_info() {
        ((boot_source=$(cat $BIOS_BOOT_CHIP | head -n 1)))
        ((boot_status=$(cat $BIOS_BOOT_STATUS | head -n 1)))

        if [ $boot_status -eq 1 ]; then
                echo "COMe CPU boots OK"
        else
                echo "COMe CPU boots not OK"
        fi

        if [ $boot_source -eq 0 ]; then
                echo "COMe CPU boots from BIOS Master flash"
        else
                echo "COMe CPU boots from BIOS Slave flash"
        fi
}

cpld_upgrade() {
	if [ $# -lt 2 ]; then
		echo "cpld_upgrade [loop1/loop2/loop3] [image_path]"
		return 1
	fi
	source /usr/local/bin/openbmc-utils.sh
	boardtype=$(board_type)
	if [ $boardtype = "Fishbone32" ] || [ $boardtype = "Fishbone48" ]; then
		if [ $1 != "loop1" ]; then
			echo "cpld_upgrade [loop1] [image_path]"
			return 1
		fi
		if [ -e $2 ]; then
			gpio_set L2 1
			ispvm –f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
			gpio_set L2 0
		fi
	elif [ $boardtype = "Phalanx" ]; then
		if [ $1 = "loop1" ]; then
			if [ -e $2 ]; then
				gpio_set L2 1
				gpio_set P0 0
				ispvm –f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
				gpio_set L2 0
				gpio_set P0 0
				gpio_set O0 0
			fi
		elif [ $1 = "loop2" ]; then
			if [ -e $2 ]; then
				gpio_set L2 1
				gpio_set P0 1
				gpio_set O0 0
				ispvm –f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
				gpio_set L2 0
				gpio_set P0 0
				gpio_set O0 0
			fi
		elif [ $1 = "loop3" ]; then
			if [ -e $2 ]; then
				gpio_set L2 1
				gpio_set P0 1
				gpio_set O0 1
				ispvm –f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
				gpio_set L2 0
				gpio_set P0 0
				gpio_set O0 0
			fi
		else
			echo "cpld_upgrade [loop1/loop2/loop3] [image_path]"
		fi
	else 
		echo "Board not support"
	fi
}

BCM5387_reset() {
	echo 0 > $SYSLED_BCM5387_RST_SYSFS
	sleep 1
	echo 1 > $SYSLED_BCM5387_RST_SYSFS
}

sys_temp_usage(){
	echo "option: "
	echo "<cpu| switch| optical> #Chip selection"
	echo "<input| max| max_hyst> #Chip temperaure params"
	echo "<#value> #Chip value"
	echo 
}

sys_temp_show() {
	sensors syscpld-i2c-0-0d
}

sys_temp() {
	if [ $# -eq 0 ]; then
		sys_temp_show
		return 0
	fi

	if [ $# -lt 3 ]; then
		sys_temp_usage
		return 1
	fi

	if [ "$1" == "switch" ]; then
		file_prefix="temp1_"
	elif [ "$1" == "cpu" ]; then
		file_prefix="temp2_"
	elif [ "$1" == "optical" ]; then
		file_prefix="temp3_"
	else
		sys_temp_usage
		return 1
	fi

	if [ "$2" == "input" ]; then
		file_suffix="input"
	elif [ "$2" == "max" ]; then
		file_suffix="max"
	elif [ "$2" == "max_hyst" ]; then
		file_suffix="max_hyst"
	else
		sys_temp_usage
		return 1
	fi

	((value=${3}*1000))
	echo $value > $SYSCPLD_SYSFS_DIR/$file_prefix$file_suffix
	return 0
}

come_rest_status() {
	silent=0
	ret=0
	if [ $# -ge 1 ]; then
		silent=$1
	fi

	val=$(cat $COME_RESET_STATUS_SYSFS 2> /dev/null | head -n 1)
	case "$val" in
		0x11)
			info="Last reset type is POWER_ON_RESET"
	        ret=0 #0x11 powered on reset
			;;
		0x22)
			info="Last reset type is Software trigger WARM_RESET"
	        ret=1 #0x22 Software trigger CPU warm reset
			;;
		0x33)
			info="Last reset type is Software trigger COLD_RESET"
	        ret=2 #0x33 Software trigger CPU cold reset
			;;
		0x44)
			info="Last reset type is WARM_RESET"
	        ret=3 #0x44 CPU warm reset
			;;
		0x55)
			info="Last reset type is COLD_RESET"
	        ret=4 #0x55 CPU cold reset
			;;
		0x66)
			info="Last reset type is WDT_RESET"
	        ret=5 #0x66 CPU watchdog reset
			;;
		0x77)
			info="Last reset type is POWER_CYCLE"
	        ret=6 #0x77 CPU power cycle
			;;
		*)
			info="Last reset type is POWER_ON_RESET"
			ret=0 #default power on reset
			;;
	esac

	if [ $silent -eq 0 ]; then
		echo "$info"
	elif [ $silent -eq 2 ]; then
		logger -p user.crit "$info"
	fi

	return $ret
}

get_cpu_temp() {
    TJMAX=$(peci-util 0x30 0x05 0x05 0xa1 0x00 16 00 00 | awk -F " " '{printf "0x%s\n", $4}')
    info=$(peci-util 0x30 0x1 0x2 0x01)
    lsb=$(echo $info |awk -F " " '{printf "0x%s\n", $1}')
    msb=$(echo $info |awk -F " " '{printf "0x%s\n", $2}')
    ((sign=$msb&0x80))
    if [ $sign -ne 128 ] ; then
        return 1
    fi
    ((a=($lsb&0xc0)>>6))
    ((b=($msb<<2)+$a))
    ((t=((~$b)+1)&0xff))
    ((c=$lsb&0x3f))
    if [ $c -ge 40 ]; then
        ((t=$t+1))
    fi
    ((temp=$TJMAX-$t))
    echo $temp

    return 0
}

upgrade_syslog_server() {
    port=514
    if [ $# -ge 2 ]; then
        port=$2
    fi
    str="*.* action(type=\"omfwd\" target=\"$1\" port=\"$port\" template=\"ForwardFormatInContainer\" protocol=\"udp\""
    num=$(sed -n -e '/target=/=' /etc/rsyslog.conf)
    if [ ! -n $num ]; then
        echo "fail"
    fi
    cmd="${num}c $str"
    sed -i -e "$cmd" /etc/rsyslog.conf
    if [ $? -eq 0 ]; then
        /etc/init.d/syslog.rsyslog restart
        echo "success"
    else
        echo "fail"
    fi
}

set_hwmon_value() {
	echo ${5} > /sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/hwmon/hwmon${3}/${4} 2> /dev/null
}

get_hwmon_id() {
	path="/sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/"
	str=$(find $path -name "$3")
	id=$(echo $str | awk -F 'hwmon' '{print $3}' | awk -F '/' '{print $1}')
	if [ $id ]; then
		if [ "$id" -gt 0 ] 2>/dev/null; then
			echo $id
		else
			echo 0
		fi
		return 0
	fi
	echo 0
}

set_hwmon_threshold() {
    id=$(get_hwmon_id $1 $2 in1_min)
    if [ "$id" -gt "0" ] ; then
        set_hwmon_value $1 $2 $id $3 $4
        if [ $? -eq 0 ]; then
            echo "0"
        else
            echo "-1"
        fi
    else
        echo "-1"
    fi
}

bmc_reboot() {
    slave=0
    if [ $# -lt 1 ] ; then
        return 1
    fi
    if /usr/local/bin/boot_info.sh |grep "Slave Flash" ; then
        slave=1
    else
        slave=0
    fi
    if [ "$1" == "master" ]; then
        if [ $slave -eq 1 ]; then #current is slave booting
            devmem_set_bit 0x1e78502c 7
            logger -p user.warning "Set BMC booting flash to master"
        fi
        if [ -f /var/log/boot_slave ]; then
            rm /var/log/boot_slave
            sync
        fi
        return 0
    elif [ "$1" == "slave" ]; then
        if [ $slave -eq 0 ]; then ##current is master booting
            devmem_set_bit 0x1e78502c 7
            logger -p user.warning "Set BMC booting flash to slave"
        fi
        echo 1 > /var/log/boot_slave
        sync
        return 0
    elif [ "$1" == "reboot" ]; then
        ((val=$(devmem 0x1e78502c)))
        ((ret=$val&0x80))
        if [ $ret -gt 0 ]; then
            if [ $slave -eq 1 ]; then #current is slave, switch to master
                logger -p user.warning "BMC will be booting from master flash"
                boot_from master
            else
                logger -p user.warning "BMC will be booting from slave flash"
                boot_from slave
            fi
        else
            logger -p user.warning "BMC will be booting from current flash"
            reboot #not set boot flash, just reboot bmc
        fi
        return 0
    fi
    return 1
}

cpld_refresh() {
    ret=1
	if [ $# -lt 1 ]; then
		echo "cpld_refresh <type> [image_path]"
		return 1
	fi
    logger -p user.info "cpld_refresh para: $@"
	boardtype=$(board_type)
	if [ "$boardtype" = "Fishbone32" ] || [ "$boardtype" = "Fishbone48" ]; then
        #power off CPU
        logger -p user.warning "cpld_refresh: power off CPU"
        /usr/local/bin/wedge_power.sh off
        sleep 1
        logger -p user.warning "cpld_refresh: power on Switch"
        i2cset -f -y 0 0x0d 0x40 0x1
        sleep 3

        if [ $# -ge 2 ]; then
            logger -p user.warning "cpld_refresh: start $1 CPLD refreshing"
		    gpio_set L2 1
		    #ispvm -f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
		    ispvm dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
            ret=$?
		    gpio_set L2 0
            logger -p user.warning "cpld_refresh: done, result=$ret"
        fi
        sleep 5
        if [ $# -ge 4 ]; then
            logger -p user.warning "cpld_refresh: start $3 CPLD refreshing"
		    gpio_set L2 1
		    #ispvm -f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $4 --tdo 212 --tdi 213 --tms 214 --tck 215
		    ispvm dll /usr/lib/libcpldupdate_dll_gpio.so $4 --tdo 212 --tdi 213 --tms 214 --tck 215
            ret=$?
		    gpio_set L2 0
            logger -p user.warning "cpld_refresh: done, result=$ret"
        fi
        sleep 5
        logger -p user.warning "cpld_refresh: power cycle CPU"
        /usr/local/bin/wedge_power.sh cycle
        logger -p user.warning "cpld_refresh: BMC rebooting"
        reboot
	elif [ $boardtype = "Phalanx" ]; then
        #power off CPU
        logger -p user.warning "cpld_refresh: power off CPU"
        /usr/local/bin/wedge_power.sh off
        sleep 1
		if [ $# -ge 2 ]; then
            logger -p user.warning "cpld_refresh: start $1 CPLD refreshing"
			gpio_set L2 1
			gpio_set P0 0
			#ispvm -f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
			ispvm dll /usr/lib/libcpldupdate_dll_gpio.so $2 --tdo 212 --tdi 213 --tms 214 --tck 215
            ret=$?
			gpio_set L2 0
			gpio_set P0 0
			gpio_set O0 0
            logger -p user.warning "cpld_refresh: done, result=$ret"
		fi
        sleep 5
		if [ $# -ge 4 ]; then
            logger -p user.warning "cpld_refresh: start $3 CPLD refreshing"
			gpio_set L2 1
			gpio_set P0 0
			#ispvm -f 1000 dll /usr/lib/libcpldupdate_dll_gpio.so $4 --tdo 212 --tdi 213 --tms 214 --tck 215
			ispvm dll /usr/lib/libcpldupdate_dll_gpio.so $4 --tdo 212 --tdi 213 --tms 214 --tck 215
            ret=$?
			gpio_set L2 0
			gpio_set P0 0
			gpio_set O0 0
            logger -p user.warning "cpld_refresh: done, result=$ret"
		fi
        sleep 5
        logger -p user.warning "cpld_refresh: power cycle CPU"
        /usr/local/bin/wedge_power.sh cycle
        logger -p user.warning "cpld_refresh: BMC rebooting"
        reboot
	else
		echo "Board not support"
	fi
    return $ret
}
