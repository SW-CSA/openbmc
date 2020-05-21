#!/usr/bin/python

import os
import re
import sys
import time
import syslog


PSU_NUM = 4
IR358X_NUM = 20
TEMP_NUM = 11
MONITOR_POLL_TIME = (60 * 10) #10 mins
MonitorItem = [
######sensors#######
	['PSU', 'dps1100-i2c-27-58', 'dps1100-i2c-26-58', 'dps1100-i2c-25-58', 'dps1100-i2c-24-58'], #PSU
	['IR358x', 'ir3584-i2c-4-15', 'ir3584-i2c-4-16', 'ir38062-i2c-4-42', 
	'ir3584-i2c-16-70', 'ir38062-i2c-16-49', 
	'ir38060-i2c-17-45', 'ir38062-i2c-17-49', 
	'ir3584-i2c-19-30', 'ir3584-i2c-19-50', 'ir3584-i2c-19-70', 
	'ir3584-i2c-20-50', 'ir3584-i2c-20-70', 'ir38060-i2c-20-45', 
	'ir3584-i2c-21-30', 'ir3584-i2c-21-50', 'ir3584-i2c-21-70', 
	'ir3584-i2c-22-50', 'ir3584-i2c-22-70', 'ir38060-i2c-22-45', 
	'ir38060-i2c-23-45'], #IR358x
	#['Temp', 'tmp75-i2c-7-4d', 'tmp75-i2c-7-4e', 'tmp75-i2c-7-4f', 
	#'tmp75-i2c-31-48', 'tmp75-i2c-31-49', 
	#'tmp75-i2c-39-48', 'tmp75-i2c-39-49', 
	#'tmp75-i2c-42-48', 'tmp75-i2c-42-49', 
	#'tmp75-i2c-43-48', 'tmp75-i2c-43-49'], #Temp
]

psu_obj = [0]*PSU_NUM
ir358x_obj = [0]*IR358X_NUM
temp_obj = [0]*TEMP_NUM

INITIAL_VALUE = 123456
VARIABLE_DISABLE = 515151
VALID_VALUE = 0
INVALID_VALUE = -1
UPPER_VALUE = 1
LOWER_VALUE = 2

psu_name_map = [
	['vin'   , 'Input Voltage'],
	['vout1' , 'Output Voltage'],
	['pin'   , 'Input Power'],
	['pout1' , 'Output Power'],
	['iin'   , 'Input Current'],
	['iout1' , 'Output Current'],
]

powerchip_name_map = [
	['ir3584-i2c-4-15'     , 'CPU_CORE VCCIN_1.82V(CPU core 1.82V'],
	['ir3584-i2c-4-16'     , 'CPU_VCC/VCCIO_1.05V(CPU 1.05V voltage'],
	['ir38062-i2c-4-42'    , 'Baseboard_Standby_3.3V Voltage(Baseboard standby 3.3V'],
	['ir3584-i2c-16-70'    , 'Switch_TRVDD_0.8V Voltage(Switch TRVDD 0.8V'],
	['ir38062-i2c-16-49'   , 'Switch_TVDD_1.2V(Switch TVDD 1.2V'],
	['ir38060-i2c-17-45'   , 'Switch_FPGA_1.0V Voltage(Switch FPGA 1.0V'],
	['ir38062-i2c-17-49'   , 'Switch_PVDD_0.8V Voltage(Switch PVDD 0.8V'],
	['ir3584-i2c-19-30'    , 'TOP_LC_port&CPLD_Supply_3.3V(TOP Linecard 3.3V'],
	['ir3584-i2c-19-50'    , 'TOP_LC_VDD_A_0.8(TOP Linecard left DVDD 0.8V'],
	['ir3584-i2c-19-70'    , 'TOP_LC_DVDDM_A_0.8V(TOP Linecard left DVDDM 0.8V'],
	['ir3584-i2c-20-50'    , 'TOP_LC_VDD_B_0.8V(TOP Linecard right DVDD 0.8V'],
	['ir3584-i2c-20-70'    , 'TOP_LC_DVDDM_B_0.8V(TOP Linecard right DVDDM 0.8V'],
	['ir38060-i2c-20-45'   , 'TOP_LC_VDDIO_1.8V Voltage(TOP Linecard I/O 1.8V'],
	['ir3584-i2c-21-30'    , 'BOTTOM_LC_port&CPLD_Supply_3.3V(BOTTOM Linecard 3.3V'],
	['ir3584-i2c-21-50'    , 'BOTTOM_LC_VDD_A_0.8V(BOTTOM Linecard left DVDD 0.8V'],
	['ir3584-i2c-21-70'    , 'BOTTOM_LC_DVDDM_A_0.8V(BOTTOM Linecard left DVDDM 0.8V'],
	['ir3584-i2c-22-50'    , 'BOTTOM_LC_VDD_B_0.8V(BOTTOM Linecard right DVDD 0.8V'],
	['ir3584-i2c-22-70'    , 'BOTTOM_LC_DVDDM_A_0.8V(BOTTOM Linecard right DVDDM 0.8V'],
	['ir38060-i2c-22-45'   , 'BOTTOM_LC_VDDIO_1.8V(BOTTOM Linecard I/O 1.8V'],
	['ir38060-i2c-23-45'   , 'Switch_Standby_3.3V(Switch standby 3.3V'],	
]

class Alarm_Data():
	def __init__(self, s):
		self.alarm_name = s
		self.value = INITIAL_VALUE;
		self.alarm_min = INITIAL_VALUE
		self.alarm_max = INITIAL_VALUE
		self.alarm_max_hyst = INITIAL_VALUE
		self.fail = 0;

	def set(self, min_val, max_val):
		self.alarm_min = min_val
		self.alarm_max = max_val

	def set_temp(self, min_val, hyst_val):
		self.alarm_max = max_val
		self.alarm_max_hyst = hyst_val

	def get_min(self):
		return self.alarm_min

	def get_max(self):
		return self.alarm_max

	def get_max_hyst(self):
		return self.alarm_max_hyst

	def get_value(self, strline):
		#cmd = 'echo ' + strline + ' |  awk -F \'+\' \'{print $2}\' |awk -F \' \' \'{print $1}\''
		cmd = 'val=$(echo \"' + strline + '\" |  awk -F \':\' \'{print $2}\' |awk -F \' \' \'{print $1}\');echo ${val#+}'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv != '':
			if recv.strip() != 'N/A':
				self.value = recv.strip()
			else:
				self.value = INVALID_VALUE
		else:
			self.value = INVALID_VALUE


	def get_threshold(self, strline):
		#cmd = 'echo ' + strline + ' |  awk -F \'+\' \'{print $3}\' |awk -F \' \' \'{print $1}\''
		cmd = 'val=$(echo \"' + strline + '\" |  awk -F \'=\' \'{print $2}\' |awk -F \' \' \'{print $1}\');echo ${val#+}'
		sys.stdout.flush()
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv != '':
			self.alarm_min = recv.strip()
		else:
			self.alarm_min = INVALID_VALUE

		#cmd = 'echo ' + strline + ' |  awk -F \'+\' \'{print $4}\' |awk -F \' \' \'{print $1}\''
		cmd = 'val=$(echo \"' + strline + '\" |  awk -F \'=\' \'{print $3}\' |awk -F \' \' \'{print $1}\');echo ${val#+}'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv != '':
			self.alarm_max = recv.strip()
		else:
			self.alarm_max = INVALID_VALUE

		syslog.syslog(syslog.LOG_INFO, self.alarm_name + ': (min: ' + str(self.alarm_min) + ', max: ' + str(self.alarm_max) + ')')

	def get_temp_threshold(self, strline):
		self.alarm_min = VARIABLE_DISABLE
		#cmd = 'echo ' + strline + ' |  awk -F \'+\' \'{print $3}\' |awk -F \' \' \'{print $1}\''
		cmd = 'val=$(echo \"' + strline + '\" |  awk -F \'=\' \'{print $2}\' |awk -F \' \' \'{print $1}\');echo ${val#+}'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv != '':
			self.alarm_max = recv.strip()
		else:
			self.alarm_max = INVALID_VALUE

		#cmd = 'echo ' + strline + ' |  awk -F \'+\' \'{print $4}\' |awk -F \' \' \'{print $1}\''
		cmd = 'val=$(echo \"' + strline + '\" |  awk -F \'=\' \'{print $3}\' |awk -F \' \' \'{print $1}\');echo ${val#+}'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv != '':
			self.alarm_max_hyst = recv.strip()
		else:
			self.alarm_max_hyst = INVALID_VALUE
		syslog.syslog(syslog.LOG_INFO, self.alarm_name + ': (max: ' + str(self.alarm_max) + ', hyst: ' + str(self.alarm_max_hyst) + ')')

	def get_power_threshold(self, strline):
		self.alarm_min = 0
		self.alarm_max_hyst = VARIABLE_DISABLE
		cmd = 'val=$(echo \"' + strline + '\" |  awk -F \'=\' \'{print $2}\' |awk -F \' \' \'{print $1}\');echo ${val#+}'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv != '':
			cmd = 'echo \"' + strline + '\" |  awk -F \'=\' \'{print $2}\' '
			sys.stdout.flush()
			s = os.popen(cmd).read()
			if s.find('kW') >= 0:
				self.alarm_max = ('%.2f' % (float(recv.strip()) * 1000))
			else:
				self.alarm_max = recv.strip()
		else:
			self.alarm_max = INVALID_VALUE
		syslog.syslog(syslog.LOG_INFO, self.alarm_name + ': (max: ' + str(self.alarm_max) + ')')


	def check_valid(self):
		if float(self.value) == float(INITIAL_VALUE) or float(self.alarm_min) == float(INITIAL_VALUE) or float(self.alarm_max) == float(INITIAL_VALUE):
			return -1 #error

		if float(self.alarm_min) == float(VARIABLE_DISABLE):
			if float(self.value) >= float(self.alarm_max):
				return UPPER_VALUE
			elif float(self.alarm_max_hyst) != float(VARIABLE_DISABLE) and float(self.value) >= float(self.alarm_max_hyst):
				return UPPER_VALUE
			else:
				return VALID_VALUE

		if float(self.alarm_max) == float(VARIABLE_DISABLE):
			if float(self.value) <= float(self.alarm_min):
				return LOWER_VALUE
			else:
				return VALID_VALUE

		if float(self.value) <= float(self.alarm_min):
			return LOWER_VALUE
		elif float(self.value) >= float(self.alarm_max):
			return UPPER_VALUE
		else:
			return VALID_VALUE

def get_mached_line(data, s):
	cmd = 'echo \"' + data + '\" | grep \"' + s + '\"'
	sys.stdout.flush()
	recv = os.popen(cmd).read()

	return recv

def get_map_name(name, alarm_name):
	matched_name = ''
	ret = re.search('dps1100', name)
	if ret:
		if name == 'dps1100-i2c-25-59':
			matched_name = 'PSU1 '
		else:
			matched_name = 'PSU2 '
		for n in  psu_name_map:
			if n[0] == alarm_name:
				matched_name += n[1]
	else:
		for n in  powerchip_name_map:
			if n[0] == name:
				matched_name = n[1] + ' ' + alarm_name.lower() + ')'

	return matched_name

def report_error(obj, alarm, ret):
	matched_name = get_map_name(obj.name, alarm.alarm_name)
	if ret == UPPER_VALUE:
		alarm.fail = 1
		syslog.syslog(syslog.LOG_CRIT, matched_name + ' exceeded upper critical, value is ' \
				+ str(alarm.value) + ', range is ' + str(alarm.alarm_min) + '~' + str(alarm.alarm_max))
	elif ret == LOWER_VALUE:
		alarm.fail = 1
		syslog.syslog(syslog.LOG_CRIT, matched_name + ' exceeded lower critical, value is ' \
				+ str(alarm.value) + ', range is ' + str(alarm.alarm_min) + '~' + str(alarm.alarm_max))
	elif ret == VALID_VALUE and alarm.fail == 1:
		alarm.fail = 0
		syslog.syslog(syslog.LOG_WARNING, matched_name + ' is NORMAL, value is ' \
				+ str(alarm.value) + ', range is ' + str(alarm.alarm_min) + '~' + str(alarm.alarm_max))


class PSU_Obj():
	def __init__(self, name):
		self.name = name
		self.in1 = Alarm_Data('vin')
		self.in2 = Alarm_Data('vout1')
		#self.fan1 = Alarm_Data('fan1')
		#self.temp1 = Alarm_Data('temp1')
		#self.temp2 = Alarm_Data('temp2')
		self.pin = Alarm_Data('pin')
		self.pout = Alarm_Data('pout1')
		self.iin = Alarm_Data('iin')
		self.iout = Alarm_Data('iout1')
		self.present = 0
		self.power_on = 0
		self.ac_ok = 0
		self.power_type = -1

	def get_psu_present(self, num):
		cmd = 'cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_' + str(PSU_NUM - num) + '_present | head -n 1'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv.strip() == '0x0':
			self.present = 1
		else:
			self.present = 0


	def get_psu_power_ok(self, num):
		cmd = 'cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_' + str(PSU_NUM - num) + '_status | head -n 1'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv.strip() == '0x1':
			if self.power_on == 0:
				syslog.syslog(syslog.LOG_WARNING, 'PSU' + psu_rename(num + 1) + ' Output Voltage status is NORMAL')
				self.power_on = 1
		else:
			if self.power_on == 1:
				syslog.syslog(syslog.LOG_CRIT, 'PSU' + psu_rename(num + 1) + ' Output Voltage status is ABNORMAL')
				self.power_on = 0


	def get_psu_ac_ok(self, num):
		cmd = 'cat /sys/bus/i2c/devices/i2c-0/0-000d/psu_' + str(PSU_NUM - num) + '_ac_status | head -n 1'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv.strip() == '0x1':
			if self.ac_ok == 0:
				syslog.syslog(syslog.LOG_WARNING, 'PSU' + psu_rename(num + 1) + ' Input Voltage status is NORMAL')
				self.ac_ok = 1
		else:
			if self.ac_ok == 1:
				syslog.syslog(syslog.LOG_CRIT, 'PSU' + psu_rename(num + 1) + ' Input Voltage status is ABNORMAL')
				self.ac_ok = 0

	def get_psu_power_type(self, num):
		min_threshold_cmd = ''
		max_threshold_cmd = ''
		if num == 0:
			cmd = 'i2cget -f -y 27 0x58 0xd8'
			min_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 27 58 in1_min'
			max_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 27 58 in1_max'
		elif num == 1:
			cmd = 'i2cget -f -y 26 0x58 0xd8'
			min_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 26 58 in1_min'
			max_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 26 58 in1_max'
		elif num == 2:
			cmd = 'i2cget -f -y 25 0x58 0xd8'
			min_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 25 58 in1_min'
			max_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 25 58 in1_max'
		elif num == 3:
			cmd = 'i2cget -f -y 24 0x58 0xd8'
			min_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 24 58 in1_min'
			max_threshold_cmd = 'source /usr/local/bin/openbmc-utils.sh;set_hwmon_threshold 24 58 in1_max'
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		if recv.strip() == '0x00':
			if self.power_type != 1:
				syslog.syslog(syslog.LOG_WARNING, 'PSU' + psu_rename(num + 1) + ' input type is AC')
				self.power_type = 1
				min_threshold_cmd += ' 90000'
				max_threshold_cmd += ' 264000'
				sys.stdout.flush()
				recv = os.popen(min_threshold_cmd).read()
				sys.stdout.flush()
				recv = os.popen(max_threshold_cmd).read()
				update_psu_threshold(num)
		elif recv.strip() == '0x01':
			if self.power_type != 2:
				syslog.syslog(syslog.LOG_WARNING, 'PSU' + psu_rename(num + 1) + ' input type is DC')
				self.power_type = 2
				min_threshold_cmd += ' 200000'
				max_threshold_cmd += ' 280000'
				sys.stdout.flush()
				recv = os.popen(min_threshold_cmd).read()
				sys.stdout.flush()
				recv = os.popen(max_threshold_cmd).read()
				update_psu_threshold(num)
		else:
			if self.power_type != 0:
				syslog.syslog(syslog.LOG_ERR, 'PSU' + psu_rename(num + 1) + ' input type is UNKNOWN')
				self.power_type = 0
				min_threshold_cmd += ' 90000'
				max_threshold_cmd += ' 264000'
				sys.stdout.flush()
				recv = os.popen(min_threshold_cmd).read()
				sys.stdout.flush()
				recv = os.popen(max_threshold_cmd).read()
				update_psu_threshold(num)


class IR358x_Obj():
	def __init__(self, name):
		self.name = name
		self.in0 = Alarm_Data('Voltage')
		self.curr1 = Alarm_Data('Current')

class TEMP_Obj():
	def __init__(self, name):
		self.name = name
		self.temp = Alarm_Data('temp')


def psu_init(item):
	i = 0
	for i in range(PSU_NUM):
		psu_obj[i] = PSU_Obj(item[i + 1])
		psu_obj[i].get_psu_present(i)
		if psu_obj[i].present == 0:
			continue
		psu_obj[i].get_psu_power_ok(i)
		if psu_obj[i].power_on == 0:
			continue

		psu_obj[i].get_psu_power_type(i)

		cmd = '/usr/bin/sensors ' + item[i + 1]
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		#print ('cmd: ' + cmd) #zmzhan add debug
		#print ('return: ' + recv) #zmzhan add debug
		if recv == '':
			print ('PSU ' + item[i + 1] + ' Init Fail!')
			continue
		else:
			in1_line = get_mached_line(recv, psu_obj[i].in1.alarm_name)
			psu_obj[i].in1.get_threshold(in1_line)

			in2_line = get_mached_line(recv, psu_obj[i].in2.alarm_name)
			psu_obj[i].in2.get_threshold(in2_line)

			pin_line = get_mached_line(recv, psu_obj[i].pin.alarm_name)
			psu_obj[i].pin.get_power_threshold(pin_line)

			pout_line = get_mached_line(recv, psu_obj[i].pout.alarm_name)
			psu_obj[i].pout.get_power_threshold(pout_line)

			iin_line = get_mached_line(recv, psu_obj[i].iin.alarm_name)
			psu_obj[i].iin.get_threshold(iin_line)

			iout_line = get_mached_line(recv, psu_obj[i].iout.alarm_name)
			psu_obj[i].iout.get_threshold(iout_line)

def update_psu_threshold(i):
	cmd = '/usr/bin/sensors ' + psu_obj[i].name
	sys.stdout.flush()
	recv = os.popen(cmd).read()
	if recv == '':
		return -1
	else:
		in1_line = get_mached_line(recv, psu_obj[i].in1.alarm_name)
		psu_obj[i].in1.get_threshold(in1_line)

		in2_line = get_mached_line(recv, psu_obj[i].in2.alarm_name)
		psu_obj[i].in2.get_threshold(in2_line)

		pin_line = get_mached_line(recv, psu_obj[i].pin.alarm_name)
		psu_obj[i].pin.get_power_threshold(pin_line)

		pout_line = get_mached_line(recv, psu_obj[i].pout.alarm_name)
		psu_obj[i].pout.get_power_threshold(pout_line)

		iin_line = get_mached_line(recv, psu_obj[i].iin.alarm_name)
		psu_obj[i].iin.get_threshold(iin_line)

		iout_line = get_mached_line(recv, psu_obj[i].iout.alarm_name)
		psu_obj[i].iout.get_threshold(iout_line)


def psu_reinit():
	i = 0
	for i in range(PSU_NUM):
		psu_obj[i].get_psu_present(i)
		if psu_obj[i].present == 0:
			continue
		psu_obj[i].get_psu_power_ok(i)
		if psu_obj[i].power_on == 0:
			continue

		psu_obj[i].get_psu_ac_ok(i)
		psu_obj[i].get_psu_power_type(i)

def psu_rename(num):
	psu_name = {
		1 : '1',
		2 : '2',
		3 : '3',
		4 : '4'
	}
	return psu_name.get(num, None)

def psu_monitor(item):
	i = 0
	for i in range(PSU_NUM):
		if psu_obj[i].present == 0 or psu_obj[i].power_on == 0:
			continue

		cmd = '/usr/bin/sensors ' + item[i + 1]
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		#print ('cmd: ' + cmd) #zmzhan add debug
		#print ('return: ' + recv) #zmzhan add debug
		if recv == '':
			print ('PSU ' + item[i + 1] + ' get info Fail!')
			continue
		else:
			in1_line = get_mached_line(recv, psu_obj[i].in1.alarm_name)
			psu_obj[i].in1.get_value(in1_line)
			psu_obj[i].in1.check_valid()
			ret = psu_obj[i].in1.check_valid()
			report_error(psu_obj[i], psu_obj[i].in1, ret)
				

			in2_line = get_mached_line(recv, psu_obj[i].in2.alarm_name)
			psu_obj[i].in2.get_value(in2_line)
			ret = psu_obj[i].in2.check_valid()
			report_error(psu_obj[i], psu_obj[i].in2, ret)

			pin_line = get_mached_line(recv, psu_obj[i].pin.alarm_name)
			psu_obj[i].pin.get_value(pin_line)
			ret = psu_obj[i].pin.check_valid()
			report_error(psu_obj[i], psu_obj[i].pin, ret)

			pout_line = get_mached_line(recv, psu_obj[i].pout.alarm_name)
			psu_obj[i].pout.get_value(pout_line)
			ret = psu_obj[i].pout.check_valid()
			report_error(psu_obj[i], psu_obj[i].pout, ret)

			iin_line = get_mached_line(recv, psu_obj[i].iin.alarm_name)
			psu_obj[i].iin.get_value(iin_line)
			ret = psu_obj[i].iin.check_valid()
			report_error(psu_obj[i], psu_obj[i].iin, ret)

			iout_line = get_mached_line(recv, psu_obj[i].iout.alarm_name)
			psu_obj[i].iout.get_value(iout_line)
			ret = psu_obj[i].iout.check_valid()
			report_error(psu_obj[i], psu_obj[i].iout, ret)

def ir358x_init(item):
	i = 0
	for i in range(IR358X_NUM):
		ir358x_obj[i] = IR358x_Obj(item[i + 1])
		cmd = '/usr/bin/sensors ' + item[i + 1]
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		#print ('cmd: ' + cmd) #zmzhan add debug
		#print ('return: ' + recv) #zmzhan add debug
		if recv == '':
			print ('IR358X ' + item[i + 1] + ' Init Fail!')
			continue
		else:
			in0_line = get_mached_line(recv, ir358x_obj[i].in0.alarm_name)
			ir358x_obj[i].in0.get_threshold(in0_line)

			curr1_line = get_mached_line(recv, ir358x_obj[i].curr1.alarm_name)
			ir358x_obj[i].curr1.get_threshold(curr1_line)

def ir358x_monitor(item):
	i = 0
	for i in range(IR358X_NUM):
		cmd = '/usr/bin/sensors ' + item[i + 1]
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		#print ('cmd: ' + cmd) #zmzhan add debug
		#print ('return: ' + recv) #zmzhan add debug
		if recv == '':
			print ('IR358X ' + item[i + 1] + ' get info Fail!')
			continue
		else:
			in0_line = get_mached_line(recv, ir358x_obj[i].in0.alarm_name)
			ir358x_obj[i].in0.get_value(in0_line)
			ret = ir358x_obj[i].in0.check_valid()
			report_error(ir358x_obj[i], ir358x_obj[i].in0, ret)

			curr1_line = get_mached_line(recv, ir358x_obj[i].curr1.alarm_name)
			ir358x_obj[i].curr1.get_value(curr1_line)
			ret = ir358x_obj[i].curr1.check_valid()
			report_error(ir358x_obj[i], ir358x_obj[i].curr1, ret)


def temp_init(item):
	i = 0
	for i in range(TEMP_NUM):
		temp_obj[i] = TEMP_Obj(item[i + 1])
		cmd = '/usr/bin/sensors ' + item[i + 1]
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		#print ('cmd: ' + cmd) #zmzhan add debug
		#print ('return: ' + recv) #zmzhan add debug
		if recv == '':
			print ('Temp ' + item[i + 1] + ' Init Fail!')
			continue
		else:
			temp_line = get_mached_line(recv, temp_obj[i].temp.alarm_name)
			temp_obj[i].temp.get_temp_threshold(temp_line)

def temp_monitor(item):
	i = 0
	for i in range(TEMP_NUM):
		cmd = '/usr/bin/sensors ' + item[i + 1]
		sys.stdout.flush()
		recv = os.popen(cmd).read()
		#print ('cmd: ' + cmd) #zmzhan add debug
		#print ('return: ' + recv) #zmzhan add debug
		if recv == '':
			print ('Temp ' + item[i + 1] + ' get info Fail!')
			continue
		else:
			temp_line = get_mached_line(recv, temp_obj[i].temp.alarm_name)
			temp_obj[i].temp.get_value(temp_line)
			ret = temp_obj[i].temp.check_valid()
			report_error(temp_obj[i], temp_obj[i].temp, ret)


def main():
	i = 0
	global psu_obj
	#print ('TestItem len:' + str(len))
	syslog.openlog("Power_monitor")
	for i in range(len):
		item = MonitorItem[i]
		switcher = {
			'PSU':psu_init,
			'IR358x':ir358x_init,
		}
		func = switcher.get(item[0], item)
		if func:
			func(item)
	while 1:
		time.sleep(MONITOR_POLL_TIME)
		psu_reinit()
		psu_monitor(MonitorItem[0])
		ir358x_monitor(MonitorItem[1])



len = len(MonitorItem)
rc = main()


