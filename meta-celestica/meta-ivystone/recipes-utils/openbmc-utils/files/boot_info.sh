#!/bin/sh

# get watch dog1 timeout status register
wdt1=$(devmem 0x1e785010)

# get watch dog2 timeout status register
wdt2=$(devmem 0x1e785030)

let "wdt1_timeout_cnt = ((wdt1 & 0xff00) >> 8)"
let "wdt2_timeout_cnt = ((wdt2 & 0xff00) >> 8)"
let "boot_code_source = ((wdt2 & 0x2) >> 1)"

boot_source='Master Flash'
if [ $boot_code_source -eq 1 ]
then
  boot_source='Slave Flash'
fi

echo "WDT1 Timeout Count: " $wdt1_timeout_cnt
echo "WDT2 Timeout Count: " $wdt2_timeout_cnt
echo "Current Boot Code Source: " $boot_source

exit 0

