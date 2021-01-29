#!/bin/bash

ratio=( 1 1 2 2 2 2 2 2 1 2 )
adc_value=( 0 0 0 0 )
for index in $(seq 0 9)
do
    echo -n "COME_POWER${index}: "
    adc_raw=$(cat /sys/bus/i2c/devices/4-0035/iio:device0/in_voltage${index}_raw)
    (( adc_raw = $adc_raw / ${ratio[$index]} ))
    for i in $(seq 0 3)
    do
        (( adc_value[$i] = $adc_raw%10 ))
        (( adc_raw = $adc_raw / 10 ))
    done
    echo "${adc_value[3]}.${adc_value[2]}${adc_value[1]}" "V"
done

