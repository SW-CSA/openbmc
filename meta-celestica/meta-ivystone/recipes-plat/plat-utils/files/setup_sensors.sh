#!/bin/sh
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

set_value() {
	echo ${4} > /sys/bus/i2c/devices/i2c-${1}/${1}-00${2}/${3} 2> /dev/null
}


#func    bus addr node val
set_value 4 15 in0_min 1600
set_value 4 15 in0_max 1950
set_value 4 15 curr1_min 0
set_value 4 15 curr1_max 43000
set_value 4 15 in0_label "CPU_Core VCCIN_1.82V Voltage"
set_value 4 15 curr1_label "CPU_Core VCCIN_1.82V Current"

set_value 4 16 in0_min 1000
set_value 4 16 in0_max 1100
set_value 4 16 curr1_min 0
set_value 4 16 curr1_max 14000
set_value 4 16 in0_label "CPU_VCC/VCCIO_1.05V Voltage"
set_value 4 16 curr1_label "CPU_VCC/VCCIO_1.05V Current"

set_value 4 42 in0_label "Baseboard_Standby_3.3V Voltage"
set_value 4 42 in0_max 3465
set_value 4 42 in0_min 3135
set_value 4 42 curr1_label "Baseboard_Standby_3.3V Current"
set_value 4 42 curr1_max 6000
set_value 4 42 curr1_min 0

set_value 7 4c temp1_max_hyst 90000
set_value 7 4c temp1_max 110000
set_value 7 4c temp2_max_hyst 90000
set_value 7 4c temp2_max 110000
set_value 7 4c temp3_max_hyst 90000
set_value 7 4c temp3_max 110000

#Fan1-5
set_value 8 0d fan1_min 1000
set_value 8 0d fan1_max 18000
set_value 8 0d fan2_min 1000
set_value 8 0d fan2_max 18000
set_value 8 0d fan3_min 1000
set_value 8 0d fan3_max 18000
set_value 8 0d fan4_min 1000
set_value 8 0d fan4_max 18000
set_value 8 0d fan5_min 1000
set_value 8 0d fan5_max 18000
set_value 8 0d fan6_min 1000
set_value 8 0d fan6_max 18000
set_value 8 0d fan7_min 1000
set_value 8 0d fan7_max 18000
set_value 8 0d fan8_min 1000
set_value 8 0d fan8_max 18000
set_value 8 0d fan9_min 1000
set_value 8 0d fan9_max 18000
set_value 8 0d fan10_min 1000
set_value 8 0d fan10_max 18000

set_value 16 70 in0_label "Switch_TRVDD_0.8V Voltage"
set_value 16 70 in0_max 840
set_value 16 70 in0_min 760
set_value 16 70 curr1_label "Switch_TRVDD_0.8V Current"
set_value 16 70 curr1_max 75000
set_value 16 70 curr1_min 0
set_value 16 49 in0_label "Switch_TVDD_1.2V Voltage"
set_value 16 49 in0_max 1260
set_value 16 49 in0_min 1140
set_value 16 49 curr1_label "Switch_TVDD_1.2V Current"
set_value 16 49 curr1_max 6000
set_value 16 49 curr1_min 0
set_value 17 45 in0_label "Switch_FPGA_1.0V Voltage"
set_value 17 45 in0_max 1050
set_value 17 45 in0_min 950
set_value 17 45 curr1_label "Switch_FPGA_1.0V Current"
set_value 17 45 curr1_max 1500
set_value 17 45 curr1_min -100
set_value 17 49 in0_label "Switch_PVDD_0.8V Voltage"
set_value 17 49 in0_max 880
set_value 17 49 in0_min 760
set_value 17 49 curr1_label "Switch_PVDD_0.8V Current"
set_value 17 49 curr1_max 15000
set_value 17 49 curr1_min 0
set_value 19 30 in0_label "TOP_LC_port&CPLD_Supply_3.3V Voltage"
set_value 19 30 in0_max 1732
set_value 19 30 in0_min 1568
set_value 19 30 curr1_label "TOP_LC_port&CPLD_Supply_3.3V Current"
set_value 19 30 curr1_max 75000
set_value 19 30 curr1_min 0
set_value 19 50 in0_label "TOP_LC_VDD_A_0.8V Voltage"
set_value 19 50 in0_max 840
set_value 19 50 in0_min 760
set_value 19 50 curr1_label "TOP_LC_VDD_A_0.8V Current"
set_value 19 50 curr1_max 100000
set_value 19 50 curr1_min 0
set_value 19 70 in0_label "TOP_LC_DVDDM_A_0.8V Voltage"
set_value 19 70 in0_max 840
set_value 19 70 in0_min 760
set_value 19 70 curr1_label "TOP_LC_DVDDM_A_0.8V Current"
set_value 19 70 curr1_max 75000
set_value 19 70 curr1_min 0
set_value 20 50 in0_label "TOP_LC_VDD_B_0.8V Voltage"
set_value 20 50 in0_max 840
set_value 20 50 in0_min 760
set_value 20 50 curr1_label "TOP_LC_VDD_B_0.8V Current"
set_value 20 50 curr1_max 100000
set_value 20 50 curr1_min 0
set_value 20 70 in0_label "TOP_LC_DVDDM_B_0.8V Voltage"
set_value 20 70 in0_max 840
set_value 20 70 in0_min 760
set_value 20 70 curr1_label "TOP_LC_DVDDM_B_0.8V Current"
set_value 20 70 curr1_max 75000
set_value 20 70 curr1_min 0
set_value 20 45 in0_label "TOP_LC_VDDIO_1.8V Voltage"
set_value 20 45 in0_max 1890
set_value 20 45 in0_min 1710
set_value 20 45 curr1_label "TOP_LC_VDDIO_1.8V Current"
set_value 20 45 curr1_max 2000
set_value 20 45 curr1_min -100
set_value 21 30 in0_label "BOTTOM_LC_port&CPLD_Supply_3.3V Voltage"
set_value 21 30 in0_max 1732
set_value 21 30 in0_min 1568
set_value 21 30 curr1_label "BOTTOM_LC_port&CPLD_Supply_3.3V Current"
set_value 21 30 curr1_max 75000
set_value 21 30 curr1_min 0
set_value 21 50 in0_label "BOTTOM_LC_VDD_A_0.8V Voltage"
set_value 21 50 in0_max 840
set_value 21 50 in0_min 760
set_value 21 50 curr1_label "BOTTOM_LC_VDD_A_0.8V Current"
set_value 21 50 curr1_max 100000
set_value 21 50 curr1_min 0
set_value 21 70 in0_label "BOTTOM_LC_DVDDM_A_0.8V Voltage"
set_value 21 70 in0_max 840
set_value 21 70 in0_min 760
set_value 21 70 curr1_label "BOTTOM_LC_DVDDM_A_0.8V Current"
set_value 21 70 curr1_max 75000
set_value 21 70 curr1_min 0
set_value 22 50 in0_label "BOTTOM_LC_VDD_B_0.8V Voltage"
set_value 22 50 in0_max 840
set_value 22 50 in0_min 760
set_value 22 50 curr1_label "BOTTOM_LC_VDD_B_0.8V Current"
set_value 22 50 curr1_max 100000
set_value 22 50 curr1_min 0
set_value 22 70 in0_label "BOTTOM_LC_DVDDM_B_0.8V Voltage"
set_value 22 70 in0_max 840
set_value 22 70 in0_min 760
set_value 22 70 curr1_label "BOTTOM_LC_DVDDM_B_0.8V Current"
set_value 22 70 curr1_max 75000
set_value 22 70 curr1_min 0
set_value 22 45 in0_label "BOTTOM_LC_VDDIO_1.8V Voltage"
set_value 22 45 in0_max 1890
set_value 22 45 in0_min 1710
set_value 22 45 curr1_label "BOTTOM_LC_VDDIO_1.8V Current"
set_value 22 45 curr1_max 2000
set_value 22 45 curr1_min -100
set_value 23 45 in0_label "Switch_Standby_3.3V Voltage"
set_value 23 45 in0_max 3465
set_value 23 45 in0_min 3135
set_value 23 45 curr1_label "Switch_Standby_3.3V Current"
set_value 23 45 curr1_max 6000
set_value 23 45 curr1_min 0

val=$(get_hwmon_id 17 60 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 17 60 $val in1_min 11400
	set_hwmon_value 17 60 $val in1_max 12600
	set_hwmon_value 17 60 $val in2_min 720
	set_hwmon_value 17 60 $val in2_max 930
	set_hwmon_value 17 60 $val temp1_max_hyst 85000
	set_hwmon_value 17 60 $val temp1_max 125000
	set_hwmon_value 17 60 $val power1_max 350000000
	set_hwmon_value 17 60 $val power2_max 350000000
	set_hwmon_value 17 60 $val curr1_min 0
	set_hwmon_value 17 60 $val curr1_max 31000
	set_hwmon_value 17 60 $val curr2_min 0
	set_hwmon_value 17 60 $val curr2_max 410000
fi

#PSU4
#add it to sensors.config
val=$(get_hwmon_id 24 58 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 24 58 $val in1_min 90000
	set_hwmon_value 24 58 $val in1_max 264000
	set_hwmon_value 24 58 $val in2_min 10800
	set_hwmon_value 24 58 $val in2_max 13200
	set_hwmon_value 24 58 $val fan1_min 1000
	set_hwmon_value 24 58 $val fan1_max 30000
	set_hwmon_value 24 58 $val temp1_max_hyst 60000
	set_hwmon_value 24 58 $val temp1_max 70000
	set_hwmon_value 24 58 $val temp2_max_hyst 60000
	set_hwmon_value 24 58 $val temp2_max 70000
	set_hwmon_value 24 58 $val power1_max 1222000000
	set_hwmon_value 24 58 $val power2_max 1100000000
	set_hwmon_value 24 58 $val curr1_min 0
	set_hwmon_value 24 58 $val curr1_max 7000
	set_hwmon_value 24 58 $val curr2_min 0
	set_hwmon_value 24 58 $val curr2_max 90000
fi

#PSU3
#add it to sensors.config
val=$(get_hwmon_id 25 58 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 25 58 $val in1_min 90000
	set_hwmon_value 25 58 $val in1_max 264000
	set_hwmon_value 25 58 $val in2_min 10800
	set_hwmon_value 25 58 $val in2_max 13200
	set_hwmon_value 25 58 $val fan1_min 1000
	set_hwmon_value 25 58 $val fan1_max 30000
	set_hwmon_value 25 58 $val temp1_max_hyst 60000
	set_hwmon_value 25 58 $val temp1_max 70000
	set_hwmon_value 25 58 $val temp2_max_hyst 60000
	set_hwmon_value 25 58 $val temp2_max 70000
	set_hwmon_value 25 58 $val power1_max 1222000000
	set_hwmon_value 25 58 $val power2_max 1100000000
	set_hwmon_value 25 58 $val curr1_min 0
	set_hwmon_value 25 58 $val curr1_max 7000
	set_hwmon_value 25 58 $val curr2_min 0
	set_hwmon_value 25 58 $val curr2_max 90000
fi
#PSU2
#add it to sensors.config
val=$(get_hwmon_id 26 58 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 26 58 $val in1_min 90000
	set_hwmon_value 26 58 $val in1_max 264000
	set_hwmon_value 26 58 $val in2_min 10800
	set_hwmon_value 26 58 $val in2_max 13200
	set_hwmon_value 26 58 $val fan1_min 1000
	set_hwmon_value 26 58 $val fan1_max 30000
	set_hwmon_value 26 58 $val temp1_max_hyst 60000
	set_hwmon_value 26 58 $val temp1_max 70000
	set_hwmon_value 26 58 $val temp2_max_hyst 60000
	set_hwmon_value 26 58 $val temp2_max 70000
	set_hwmon_value 26 58 $val power1_max 1222000000
	set_hwmon_value 26 58 $val power2_max 1100000000
	set_hwmon_value 26 58 $val curr1_min 0
	set_hwmon_value 26 58 $val curr1_max 7000
	set_hwmon_value 26 58 $val curr2_min 0
	set_hwmon_value 26 58 $val curr2_max 90000
fi
#PSU1
#add it to sensors.config
val=$(get_hwmon_id 27 58 in1_min)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 27 58 $val in1_min 90000
	set_hwmon_value 27 58 $val in1_max 264000
	set_hwmon_value 27 58 $val in2_min 10800
	set_hwmon_value 27 58 $val in2_max 13200
	set_hwmon_value 27 58 $val fan1_min 1000
	set_hwmon_value 27 58 $val fan1_max 30000
	set_hwmon_value 27 58 $val temp1_max_hyst 60000
	set_hwmon_value 27 58 $val temp1_max 70000
	set_hwmon_value 27 58 $val temp2_max_hyst 60000
	set_hwmon_value 27 58 $val temp2_max 70000
	set_hwmon_value 27 58 $val power1_max 1222000000
	set_hwmon_value 27 58 $val power2_max 1100000000
	set_hwmon_value 27 58 $val curr1_min 0
	set_hwmon_value 27 58 $val curr1_max 7000
	set_hwmon_value 27 58 $val curr2_min 0
	set_hwmon_value 27 58 $val curr2_max 90000
fi

#temp
val=$(get_hwmon_id 39 48 temp1_max)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 39 48 $val temp1_max 70000
	set_hwmon_value 39 48 $val temp1_max_hyst 60000
fi
val=$(get_hwmon_id 39 49 temp1_max)
if [ "$val" -gt "0" ] ; then
	set_hwmon_value 39 49 $val temp1_max 70000
	set_hwmon_value 39 49 $val temp1_max_hyst 60000
fi


# run sensors.config set command
sensors -s
sleep 3

#run power monitor
echo "Start Power monitor"
/usr/local/bin/power_monitor.py &
