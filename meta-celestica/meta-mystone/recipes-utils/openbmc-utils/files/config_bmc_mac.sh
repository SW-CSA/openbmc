#!/bin/sh

. /usr/local/bin/openbmc-utils.sh

count=0
while [ $count -lt 3 ]
do
    info=$(/usr/local/bin/fruid-util sys)
    str1=$(echo "$info" |grep "Product Custom Data 2" |awk -F ":" '{printf "%s:%s:%s:%s:%s\n",  $2,$3,$4,$5,$6}')
    str2=$(echo "$info" |grep "Product Custom Data 2" |awk -F ":" '{printf "0x%s\n", $7}')
    val=$(($str2+1))
    mac=$(printf "%s:%X\n" $str1 $val)

    if [ -n "$mac" -a "${mac/X/}" = "${mac}" ]; then
        echo "Configure BMC MAC: $mac"
		/sbin/ifdown eth0
		/sbin/ifconfig eth0 hw ether $mac
		/sbin/ifup eth0
        sleep 1
        exit 0
    fi
    count=$(($count + 1))
    sleep 1
done
if [ $count -ge 3 ]; then
    echo "Cannot find out the BMC MAC" 1>&2
    logger "Error: cannot configure the BMC MAC"
    exit -1
fi

