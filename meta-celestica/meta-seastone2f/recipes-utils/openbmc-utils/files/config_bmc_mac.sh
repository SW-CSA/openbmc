#!/bin/sh

. /usr/local/bin/openbmc-utils.sh
MAC_ADDRESS_IN_DECIMAL=0
MAC_ADDRESS=''
mac_to_num()
{
    num=0
    significant=`printf %d 0x10000000000`
    for i in {1,3,5,7,9,11}
    do
        str=`expr substr $1 $i 2`
        m=`printf %d 0x$str`
        n=`expr $m \* $significant`
        significant=`expr $significant \/ 256`
        num=`expr $num + $n`
    done
    MAC_ADDRESS_IN_DECIMAL=$num
    return 0
}
#inverse convertion
num_to_mac()
{
    num=$1
    str=''
    for i in {0..5}
    do
        n=`expr $num % 256`
        num=`expr $num \/ 256`
        n=`printf %02X $n`
        str=$n:$str
    done
    MAC_ADDRESS=${str%:}
    return 0
}

parse_mac()
{
    mac_to_num $1
    num_to_mac `expr $MAC_ADDRESS_IN_DECIMAL + 1`
    echo $MAC_ADDRESS
    MAC_ADDRESS_IN_DECIMAL=0
    MAC_ADDRESS=''
}

#up usb0 device
ifconfig usb0 up "fe80::1/64"

count=0
while [ $count -lt 3 ]
do
    info=$(/usr/local/bin/fruid-util sys)
    str1=$(echo "$info" |grep "Product Custom Data 2" |awk -F ":" '{print  $2$3$4$5$6$7}')
    str1=$(echo $str1 |sed 's/ //g')
    if [ ${#str1} -ne 12 ];then
        logger "Loop $count failed to read BMC FRU:$str1"
        count=$(($count + 1))
        sleep 1
        continue
    fi
    mac=$(parse_mac $str1)

    if [ -n "$mac" -a "${mac/X/}" = "${mac}" ]; then
        logger "Loop $count success to configure BMC MAC: $mac"
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

