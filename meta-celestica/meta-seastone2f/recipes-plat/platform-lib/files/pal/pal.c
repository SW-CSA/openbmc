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

/***   fruid-util    *****/
fru_device_T seastone2f_fru_device[] =
{
    {"all",FRU_ALL, 1, NULL,NULL,NULL},
    {"sys",FRU_SYS, 1, "/sys/bus/i2c/devices/i2c-2/2-0057/eeprom","Base Board",NULL},
    {"bmc",FRU_BMC, 1, "/sys/bus/i2c/devices/i2c-2/2-0053/eeprom","BMC Board",NULL},
    {"cpu",FRU_CPU, 1, "/sys/bus/i2c/devices/i2c-1/1-0050/eeprom","CPU Board",NULL},
    {"fb",FRU_FB, 1, "/sys/bus/i2c/devices/i2c-39/39-0056/eeprom","FAN Board",NULL},
    {"switch",FRU_SWITCH, 1, "/sys/bus/i2c/devices/i2c-2/2-0051/eeprom","Switch Board",NULL},
    {"psu1",FRU_PSU1, 1, "/sys/bus/i2c/devices/i2c-25/25-0051/eeprom","PSU1","/sys/bus/i2c/devices/i2c-0/0-000d/psu_r_present"},
    {"psu2",FRU_PSU2, 1, "/sys/bus/i2c/devices/i2c-24/24-0050/eeprom","PSU2","/sys/bus/i2c/devices/i2c-0/0-000d/psu_l_present"},
    {"psu3",FRU_PSU3, 0, NULL,NULL,NULL},
    {"psu4",FRU_PSU4, 0, NULL,NULL,NULL},
    {"fan1",FRU_FAN1, 1, "/sys/bus/i2c/devices/i2c-34/34-0050/eeprom","Fantray1","/sys/bus/i2c/devices/i2c-8/8-000d/fan4_present"},
    {"fan2",FRU_FAN2, 1, "/sys/bus/i2c/devices/i2c-32/32-0050/eeprom","Fantray2","/sys/bus/i2c/devices/i2c-8/8-000d/fan3_present"},
    {"fan3",FRU_FAN3, 1, "/sys/bus/i2c/devices/i2c-38/38-0050/eeprom","Fantray3","/sys/bus/i2c/devices/i2c-8/8-000d/fan2_present"},
    {"fan4",FRU_FAN4, 1, "/sys/bus/i2c/devices/i2c-36/36-0050/eeprom","Fantray4","/sys/bus/i2c/devices/i2c-8/8-000d/fan1_present"},
    {"fan5",FRU_FAN5, 0, NULL,NULL,NULL},
    {"fan6",FRU_FAN6, 0, NULL,NULL,NULL},
    {"lc1",FRU_LINE_CARD1, 0, NULL,NULL,NULL},
    {"lc2",FRU_LINE_CARD2, 0, NULL,NULL,NULL}
};

const char pal_fru_list[] = "all, sys, bmc, cpu, fb, switch, psu1, psu2, psu3, psu4, fan1, fan2, fan3, fan4, fan5, fan6, lc1, lc2";
static fru_device_T * fru_device = {0};
static int fru_len=0;

/***   fruid-util    *****/

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
    int i=0;
    if(pal_get_iom_board_id() == -1)
        return -1;
    for(i=0;i<fru_len;i++)
    {
        if( !strcmp(str,fru_device[i].fru_str) )
        {
            if(fru_device[i].fru_available == 0)
                return -1;
            *fru = fru_device[i].fru_id;
            return 0;
        }
    }
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru id %s", str);
    return -1;
}

int pal_get_fruid_path(uint8_t fru, char *path)
{
    return pal_get_fruid_eeprom_path(fru, path);
}

int pal_get_fruid_eeprom_path(uint8_t fru, char *path)
{
    int i=0;
    if(pal_get_iom_board_id() == -1)
        return -1;
    for(i=0;i<fru_len;i++)
    {
        if( fru == fru_device[i].fru_id )
        {
            if(fru_device[i].fru_available == 0)
                return -1;
            sprintf(path, fru_device[i].fru_path);
            return 0;
        }
    }
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru path %d", fru);
    return -1;
}

int pal_get_fruid_name(uint8_t fru, char *name)
{
    int i=0;
    if(pal_get_iom_board_id() == -1)
        return -1;
    for(i=0;i<fru_len;i++)
    {
        if( fru == fru_device[i].fru_id )
        {
            if(fru_device[i].fru_available == 0)
                return -1;
            sprintf(name, fru_device[i].fru_name);
            return 0;
        }
    }
    syslog(LOG_WARNING, "pal_get_fru_id: Wrong fru name %d", fru);
    return -1;
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
    if (fru >=FRU_PSU1 && fru <= FRU_FAN6)
    {
        int i=0;
        if(pal_get_iom_board_id() == -1)
            return -1;
        for(i=0;i<fru_len;i++)
        {
            if( fru == fru_device[i].fru_id )
            {
                if(fru_device[i].fru_available == 0)
                    return -1;
                snprintf(full_name, LARGEST_DEVICE_NAME, "%s", fru_device[i].fru_prsnt);
                break;
            }
        }
        if (read_device(full_name, &value))
            return -1;
        *status = !value;
    }
    else
    {
        *status = 1;
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
    if (iom_board_id == 0 )
    {
        //Seastone2F
        fru_device = seastone2f_fru_device;
        fru_len =  sizeof( seastone2f_fru_device ) / sizeof( fru_device_T );
        return 0;
    }
    syslog(LOG_WARNING, "pal_get_fru_id: Failed to get platform\n");
    return -1;
}