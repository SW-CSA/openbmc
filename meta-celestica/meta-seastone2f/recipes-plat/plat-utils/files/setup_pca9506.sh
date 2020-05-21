#!/bin/bash
GPIO_EXPORT="/sys/class/gpio/export"
GPIO_PATH="/sys/class/gpio/gpio"
PSU_INT_BASE=448
PSU_PSON_BASE=456
PSU_KILL_ON_BASE=464
PSU_PRESENT_BASE=472
PSU3_PW_OK=480
PSU4_PW_OK=481
PSU3_AC_OK=482
PSU4_AC_OK=483

psu_offset=( 0 1 4 5 2 3 )

for psu_index in $(seq 0 5)
do
	gpio=$((PSU_INT_BASE + ${psu_offset[$psu_index]}))
	echo $gpio > $GPIO_EXPORT
	# echo in > "${GPIO_PATH}${gpio}/direction"

	gpio=$((PSU_PSON_BASE + ${psu_offset[$psu_index]}))
	echo $gpio > $GPIO_EXPORT
	echo out > "${GPIO_PATH}${gpio}/direction"
	echo 0 > "${GPIO_PATH}${gpio}/value"

	gpio=$((PSU_KILL_ON_BASE + ${psu_offset[$psu_index]}))
	echo $gpio > $GPIO_EXPORT
	echo out > "${GPIO_PATH}${gpio}/direction"
	echo 0 > "${GPIO_PATH}${gpio}/value"

	gpio=$((PSU_PRESENT_BASE + ${psu_offset[$psu_index]}))
	echo $gpio > $GPIO_EXPORT
	# echo in > "${GPIO_PATH}${gpio}/direction"
done

gpio=${PSU3_PW_OK}
echo $gpio > $GPIO_EXPORT
# echo in > "${GPIO_PATH}${gpio}/direction"

gpio=${PSU4_PW_OK}
echo $gpio > $GPIO_EXPORT
# echo in > "${GPIO_PATH}${gpio}/direction"

gpio=${PSU3_AC_OK}
echo $gpio > $GPIO_EXPORT
# echo in > "${GPIO_PATH}${gpio}/direction"

gpio=${PSU4_AC_OK}
echo $gpio > $GPIO_EXPORT
# echo in > "${GPIO_PATH}${gpio}/direction"

