/*
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 /*
  * This file contains functions and logics that depends on Wedge100 specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>


#include "pal.h"
const char pal_fru_list[] = "all, sys, bmc, cpu, fb, switch, psu1, psu2, psu3, psu4, fan1, fan2, fan3, fan4, fan5, lc1, lc2";
static int read_device(const char *device, int *value)
{
	FILE *fp;
	int rc;

	fp = fopen(device, "r");
	if (!fp) {
		int err = errno;
#ifdef DEBUG
		syslog(LOG_INFO, "failed to open device %s", device);
#endif
		return err;
	}

	rc = fscanf(fp, "%d", value);
	fclose(fp);
	if (rc != 1) {
#ifdef DEBUG
		syslog(LOG_INFO, "failed to read device %s", device);
#endif
		return -ENOENT;
	} else {
		return 0;
	}

	return 0;
}

#if 0
static int write_device(const char *device, const char *value)
{
	FILE *fp;
	int rc;

	fp = fopen(device, "w");
	if (!fp) {
		int err = errno;
#ifdef DEBUG
		syslog(LOG_INFO, "failed to open device for write %s", device);
#endif
		return err;
	}

	rc = fputs(value, fp);
	fclose(fp);

	if (rc < 0) {
#ifdef DEBUG
		syslog(LOG_INFO, "failed to write device %s", device);
#endif
		return -ENOENT;
	} else {
		return 0;
	}

	return 0;
}
#endif

int pal_get_fru_list(char *list)
{
	strcpy(list, pal_fru_list);
	return 0;
}

int pal_get_fru_id(char *str, uint8_t *fru)
{
	if (!strcmp(str, "all")) {
		*fru = FRU_ALL;
	} else if (!strcmp(str, "sys")) {
		*fru = FRU_SYS;
	} else if (!strcmp(str, "bmc")) {
		*fru = FRU_BMC;
	} else if (!strcmp(str, "cpu")) {
		*fru = FRU_CPU;
	} else if (!strcmp(str, "fb")) {
		*fru = FRU_FB;
	} else if (!strcmp(str, "switch")) {
		*fru = FRU_SWITCH;
	} else if (!strcmp(str, "psu1")) {
		*fru = FRU_PSU1;
	} else if (!strcmp(str, "psu2")) {
		*fru = FRU_PSU2;
	} else if (!strcmp(str, "psu3")) {
		if(pal_get_iom_board_id() == 0)
			return -1;
		*fru = FRU_PSU3;
	} else if (!strcmp(str, "psu4")) {
		if(pal_get_iom_board_id() == 0)
			return -1;
		*fru = FRU_PSU4;
	} else if (!strcmp(str, "fan1")) {
		*fru = FRU_FAN1;
	} else if (!strcmp(str, "fan2")) {
		*fru = FRU_FAN2;
	} else if (!strcmp(str, "fan3")) {
		*fru = FRU_FAN3;
	} else if (!strcmp(str, "fan4")) {
		*fru = FRU_FAN4;
	} else if (!strcmp(str, "fan5")) {
		if(pal_get_iom_board_id() == 0)
			return -1;
		*fru = FRU_FAN5;
	} else if (!strcmp(str, "lc1")) {
		if(pal_get_iom_board_id() == 0)
			return -1;
		*fru = FRU_LINE_CARD1;
	} else if (!strcmp(str, "lc2")) {
		if(pal_get_iom_board_id() == 0)
			return -1;
		*fru = FRU_LINE_CARD2;
	} else if (!strncmp(str, "fru", 3)) {
		*fru = atoi(&str[3]);
		if (*fru < FRU_SYS || *fru >= MAX_NUM_FRUS)
			return -1;
	} else {
		syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru#%s", str);
		return -1;
	}

	return 0;
}

int pal_get_fruid_path(uint8_t fru, char *path)
{
    return pal_get_fruid_eeprom_path(fru, path);
}

int pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{
	switch(fru) {
		case FRU_SYS:
			sprintf(path, "/sys/bus/i2c/devices/i2c-2/2-0057/eeprom");
			break;
		case FRU_BMC:
			sprintf(path, "/sys/bus/i2c/devices/i2c-2/2-0053/eeprom");
			break;
		case FRU_CPU:
			sprintf(path, "/sys/bus/i2c/devices/i2c-1/1-0050/eeprom");
			break;
		case FRU_FB:
			sprintf(path, "/sys/bus/i2c/devices/i2c-39/39-0056/eeprom");
			break;
		case FRU_SWITCH:
			sprintf(path, "/sys/bus/i2c/devices/i2c-2/2-0051/eeprom");
			break;
		case FRU_PSU1:
			if(pal_get_iom_board_id() == 0)
				sprintf(path, "/sys/bus/i2c/devices/i2c-25/25-0051/eeprom");
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-27/27-0050/eeprom");
			break;
		case FRU_PSU2:
			if(pal_get_iom_board_id() == 0)
				sprintf(path, "/sys/bus/i2c/devices/i2c-24/24-0050/eeprom");
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-26/26-0050/eeprom");
			break;
		case FRU_PSU3:
			if(pal_get_iom_board_id() == 0)
				return -1;
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-25/25-0050/eeprom");
			break;
		case FRU_PSU4:
			if(pal_get_iom_board_id() == 0)
				return -1;
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-24/24-0050/eeprom");
			break;
		case FRU_FAN1:
			if(pal_get_iom_board_id() == 0)
				sprintf(path, "/sys/bus/i2c/devices/i2c-34/34-0050/eeprom");
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-36/36-0050/eeprom");
			break;
		case FRU_FAN2:
			if(pal_get_iom_board_id() == 0)
				sprintf(path, "/sys/bus/i2c/devices/i2c-32/32-0050/eeprom");
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-35/35-0050/eeprom");
			break;
		case FRU_FAN3:
			if(pal_get_iom_board_id() == 0)
				sprintf(path, "/sys/bus/i2c/devices/i2c-38/38-0050/eeprom");
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-34/34-0050/eeprom");
			break;
		case FRU_FAN4:
			if(pal_get_iom_board_id() == 0)
				sprintf(path, "/sys/bus/i2c/devices/i2c-36/36-0050/eeprom");
			else
				sprintf(path, "/sys/bus/i2c/devices/i2c-33/33-0050/eeprom");
			break;
		case FRU_FAN5:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(path, "/sys/bus/i2c/devices/i2c-32/32-0050/eeprom");
			break;
		case FRU_LINE_CARD1:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(path, "/sys/bus/i2c/devices/i2c-9/9-0052/eeprom");
			break;
		case FRU_LINE_CARD2:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(path, "/sys/bus/i2c/devices/i2c-10/10-0052/eeprom");
			break;
		default:
			return -1;
	}

	return 0;
}

int pal_get_fruid_name(uint8_t fru, char *name)
{
	switch(fru) {
		case FRU_SYS:
			sprintf(name, "Base Board");
			break;
		case FRU_BMC:
			sprintf(name, "BMC Board");
			break;
		case FRU_CPU:
			sprintf(name, "CPU Board");
			break;
		case FRU_FB:
			sprintf(name, "FAN Board");
			break;
		case FRU_SWITCH:
			sprintf(name, "Switch Board");
			break;
		case FRU_PSU1:
			sprintf(name, "PSU1");
			break;
		case FRU_PSU2:
			sprintf(name, "PSU2");
			break;
		case FRU_PSU3:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(name, "PSU3");
			break;
		case FRU_PSU4:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(name, "PSU4");
			break;
		case FRU_FAN1:
			sprintf(name, "Fantray1");
			break;
		case FRU_FAN2:
			sprintf(name, "Fantray2");
			break;
		case FRU_FAN3:
			sprintf(name, "Fantray3");
			break;
		case FRU_FAN4:
			sprintf(name, "Fantray4");
			break;
		case FRU_FAN5:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(name, "Fantray5");
			break;
		case FRU_LINE_CARD1:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(name, "Line Card1");
			break;
		case FRU_LINE_CARD2:
			if(pal_get_iom_board_id() == 0)
				return -1;
			sprintf(name, "Line Card2");
			break;
		default:
			return -1;
	}
	return 0;
}

int pal_get_platform_name(char *name)
{
  strcpy(name, OPENBMC_PLATFORM_NAME);
  return 0;
}

int pal_get_num_slots(uint8_t *num)
{
  *num = OPENBMC_MAX_NUM_SLOTS;
  return 0;
}

int pal_is_fru_prsnt(uint8_t fru, uint8_t *status)
{
	int value;
	char full_name[LARGEST_DEVICE_NAME + 1]={0};
	*status = 0;
	switch (fru) {
		case FRU_SYS:
		case FRU_BMC:
		case FRU_CPU:
		case FRU_FB:
		case FRU_SWITCH:
			*status = 1;
			break;
		case FRU_LINE_CARD1:
		case FRU_LINE_CARD2:
			if(pal_get_iom_board_id() == 0)
				return -1;
			*status = 1;
			break;
		case FRU_PSU1:
			if(pal_get_iom_board_id() == 0)
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-0/0-000d/psu_r_present");
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-0/0-000d/psu_1_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_PSU2:
			if(pal_get_iom_board_id() == 0)
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-0/0-000d/psu_l_present");
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-0/0-000d/psu_2_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_PSU3:
			if(pal_get_iom_board_id() == 0)
				return -1;
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-0/0-000d/psu_3_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_PSU4:
			if(pal_get_iom_board_id() == 0)
				return -1;
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-0/0-000d/psu_4_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_FAN1:
			if(pal_get_iom_board_id() == 0)
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan4_present");
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan1_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_FAN2:
			if(pal_get_iom_board_id() == 0)
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan3_present");
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan2_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_FAN3:
			if(pal_get_iom_board_id() == 0)
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan2_present");
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan3_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_FAN4:
			if(pal_get_iom_board_id() == 0)
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan1_present");
			else
				snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan4_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		case FRU_FAN5:
			if(pal_get_iom_board_id() == 0)
				return -1;
			snprintf(full_name, LARGEST_DEVICE_NAME, "%s", "/sys/bus/i2c/devices/i2c-8/8-000d/fan5_present");
			if (read_device(full_name, &value)) {
				return -1;
			}
			*status = !value;
			break;
		default:
			return -1;
	}

	return 0;
}

int pal_is_fru_ready(uint8_t fru, uint8_t *status)
{
	*status = 1;

	return 0;
}

int pal_get_iom_board_id (void)
{
	int iom_board_id = 0;
	FILE *fp;

	fp = fopen(BOARD_TYPE_PATH, "r");
	if (!fp) {
		return 0;
	}
	fscanf(fp, "%x", &iom_board_id);

	return iom_board_id;
}


