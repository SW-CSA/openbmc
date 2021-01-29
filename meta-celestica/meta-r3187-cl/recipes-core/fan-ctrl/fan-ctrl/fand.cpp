/*
 * fand
 *
 * Copyright 2016-present Celestica. All Rights Reserved.
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
 *
 * Daemon to manage the fan speed to ensure that we stay within a reasonable
 * temperature range.  We're using a simplistic algorithm to get started:
 *
 * If the fan is already on high, we'll move it to medium if we fall below
 * a top temperature.  If we're on medium, we'll move it to high
 * if the temperature goes over the top value, and to low if the
 * temperature falls to a bottom level.  If the fan is on low,
 * we'll increase the speed if the temperature rises to the top level.
 *
 * To ensure that we're not just turning the fans up, then back down again,
 * we'll require an extra few degrees of temperature drop before we lower
 * the fan speed.
 *
 * We check the RPM of the fans against the requested RPMs to determine
 * whether the fans are failing, in which case we'll turn up all of
 * the other fans and report the problem..
 *
 * TODO:  Implement a PID algorithm to closely track the ideal temperature.
 * TODO:  Determine if the daemon is already started.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <dirent.h>
#include "watchdog.h"
#include <fcntl.h>
#include <openbmc/obmc-i2c.h>

//#define FANCTRL_SIMULATION 1
//#define CONFIG_PSU_FAN_CONTROL_INDEPENDENT 1
#define CONFIG_FSC_CONTROL_PID 1 //for PID control

#define TOTAL_FANS 4
#define TOTAL_PSUS 2
#define FAN_MEDIUM 128
#define FAN_HIGH 100
#define FAN_MAX 255
#define RAISING_TEMP_LOW 26
#define RAISING_TEMP_HIGH 47
#define FALLING_TEMP_LOW 23
#define FALLING_TEMP_HIGH 44
#define SYSTEM_LIMIT 80

#define ALARM_TEMP_THRESHOLD 3
#define ALARM_START_REPORT 3

#define CRITICAL_TEMP_HYST 3

#define REPORT_TEMP 720  /* Report temp every so many cycles */
#define FAN_FAILURE_OFFSET 30
#define FAN_FAILURE_THRESHOLD 3 /* How many times can a fan fail */
#define FAN_LED_GREEN 1
#define FAN_LED_RED 2
#define SHUTDOWN_DELAY_TIME 72 /*if trigger shutdown event, delay 6mins to shutdown */

#define BAD_TEMP (-60)

#define FAN_FAIL_COUNT 3
#define FAN_FAIL_RPM 1000
#define FAN_FRONTT_SPEED_MAX 24150
#define FAN_REAR_SPEED_MAX 28950
#define PSU_SPEED_MAX 26496

#define WARN_RECOVERY_COUNT (100) //remain 5mins to recovery nomal speed

#define FAN_DIR_B2F 0
#define FAN_DIR_F2B 1

#define PWM_UNIT_MAX 255

#define PATH_CACHE_SIZE 256

#define FAN_WDT_TIME (0x3c) //5 * 60
#define FAN_WDT_ENABLE_SYSFS "/sys/bus/i2c/devices/i2c-8/8-000d/wdt_en"
#define FAN_WDT_TIME_SYSFS "/sys/bus/i2c/devices/i2c-8/8-000d/wdt_time"
#define PSU1_SHUTDOWN_SYSFS "/sys/bus/i2c/devices/i2c-24/24-0058/control/shutdown"
#define PSU2_SHUTDOWN_SYSFS "/sys/bus/i2c/devices/i2c-25/25-0059/control/shutdown"
#define PSU_SPEED_CTRL_NODE "fan1_cfg"
#define PSU_SPEED_CTRL_ENABLE 0x90

#define PID_CONFIG_PATH "/mnt/data/pid_config.ini"
#define PID_FILE_LINE_MAX 100

#define uchar unsigned char

#define LOW_WARN_BIT (0x1 << 0)
#define HIGH_WARN_BIT (0x1 << 1)
#define PID_CTRL_BIT (0x1 << 2)
#define SWITCH_SENSOR_BIT (0x1 << 3)
#define CRITICAL_SENSOR_BIT (0x1 << 4)


struct sensor_info_sysfs {
  char* prefix;
  char* suffix;
  uchar temp;
  uchar t1;
  uchar t2;
  int old_pwm;
  float setpoint;
  float p;
  float i;
  float d;
  float min_output;
  int (*read_sysfs)(struct sensor_info_sysfs *sensor);
  char path_cache[PATH_CACHE_SIZE];
};

struct fan_info_stu_sysfs {
  char *prefix;
  char *front_fan_prefix;
  char *rear_fan_prefix;
  char *pwm_prefix;
  char *fan_led_prefix;
  char *fan_present_prefix;
  //uchar present; //for chassis using, other ignore it
  uchar front_failed;  //for single fan fail
  uchar rear_failed;
};

struct psu_info_sysfs {
  char* sysfs_path;
  char* shutdown_path;
  int value_to_shutdown;
};


struct board_info_stu_sysfs {
  const char *name;
  uint slot_id;
  int correction;
  int lwarn;
  int hwarn;
  int warn_count;
  int flag;
  struct sensor_info_sysfs *critical;
  struct sensor_info_sysfs *alarm;
};

struct fantray_info_stu_sysfs {
  const char *name;
  int present;
  int failed; //for fantray fail
  struct fan_info_stu_sysfs fan1;
};

struct rpm_to_pct_map {
  uint pct;
  uint rpm;
};
struct dictionary_t {
	char name[20];
	int value;
};

struct control_level_t {
	int temp;
	int pwm;
};

struct thermal_policy_t {
	struct control_level_t level1;
	struct control_level_t level2;
	struct control_level_t level3;
	struct control_level_t level4;
	struct control_level_t level5;
	struct control_level_t level6;
	int pwm;
	int old_pwm;
};

static int read_temp_sysfs(struct sensor_info_sysfs *sensor);
static int read_temp_directly_sysfs(struct sensor_info_sysfs *sensor);


static struct sensor_info_sysfs sensor_inlet_u17_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/39-0049",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_sysfs,
};

static struct sensor_info_sysfs sensor_inlet_u52_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/39-0048",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_sysfs,
};

static struct sensor_info_sysfs sensor_bcm5870_board_u31_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-004a",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_sysfs,
};


static struct sensor_info_sysfs sensor_inlet_u28_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-004d",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_sysfs,
};

static struct sensor_info_sysfs sensor_inlet_u29_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-004c",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_sysfs,
};

static struct sensor_info_sysfs sensor_bcm5870_board_u30_critical_info = {
  .prefix = "/sys/bus/i2c/drivers/lm75/7-004b",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_sysfs,
};


static struct sensor_info_sysfs sensor_bcm5870_inlet_critical_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-0/0-000d",
  .suffix = "temp1_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_directly_sysfs,
};

static struct sensor_info_sysfs sensor_cpu_inlet_critical_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-0/0-000d",
  .suffix = "temp2_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_directly_sysfs,
};

static struct sensor_info_sysfs sensor_optical_inlet_critical_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-0/0-000d",
  .suffix = "temp3_input",
  .temp = 0,
  .t1 = 0,
  .t2 = 0,
  .old_pwm = 0,
  .setpoint = 0,
  .p = 0,
  .i = 0,
  .d = 0,
  .min_output = 0,
  .read_sysfs = &read_temp_directly_sysfs,
};


/* fantran info*/
static struct fan_info_stu_sysfs fan4_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-8/8-000d",
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "fan2_input",
  .pwm_prefix = "fan1_pwm",
  .fan_led_prefix = "fan1_led",
  .fan_present_prefix = "fan1_present",
  //.present = 1,
  .front_failed = 0,
  .rear_failed = 0,
};

static struct fan_info_stu_sysfs fan3_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-8/8-000d",
  .front_fan_prefix = "fan3_input",
  .rear_fan_prefix = "fan4_input",
  .pwm_prefix = "fan2_pwm",
  .fan_led_prefix = "fan2_led",
  .fan_present_prefix = "fan2_present",
  //.present = 1,
  .front_failed = 0,
  .rear_failed = 0,
};

static struct fan_info_stu_sysfs fan2_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-8/8-000d",
  .front_fan_prefix = "fan5_input",
  .rear_fan_prefix = "fan6_input",
  .pwm_prefix = "fan3_pwm",
  .fan_led_prefix = "fan3_led",
  .fan_present_prefix = "fan3_present",
  //.present = 1,
  .front_failed = 0,
  .rear_failed = 0,
};

static struct fan_info_stu_sysfs fan1_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-8/8-000d",
  .front_fan_prefix = "fan7_input",
  .rear_fan_prefix = "fan8_input",
  .pwm_prefix = "fan4_pwm",
  .fan_led_prefix = "fan4_led",
  .fan_present_prefix = "fan4_present",
  //.present = 1,
  .front_failed = 0,
  .rear_failed = 0,
};

static struct fan_info_stu_sysfs psu2_fan_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-0/0-000d",
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "/sys/bus/i2c/devices/i2c-24/24-0058",
  .pwm_prefix = "fan1_pct",
  .fan_led_prefix = "psu_led",
  .fan_present_prefix = "psu_l_present",
  //.present = 1,
  .front_failed = 0,
  .rear_failed = 0,
};

static struct fan_info_stu_sysfs psu1_fan_info = {
  .prefix = "/sys/bus/i2c/devices/i2c-0/0-000d",
  .front_fan_prefix = "fan1_input",
  .rear_fan_prefix = "/sys/bus/i2c/devices/i2c-25/25-0059",
  .pwm_prefix = "fan1_pct",
  .fan_led_prefix = "psu_led",
  .fan_present_prefix = "psu_r_present",
  //.present = 1,
  .front_failed = 0,
  .rear_failed = 0,
};




/************board and fantray info*****************/
static struct board_info_stu_sysfs board_info[] = {
	/*B2F*/
	{
		.name = "inlet_u17",
		.slot_id = FAN_DIR_B2F,
		.correction = -2,
		.lwarn = 45,
		.hwarn = BAD_TEMP,
		.warn_count = 0,
		.flag = CRITICAL_SENSOR_BIT,
		.critical = &sensor_inlet_u17_critical_info,
		.alarm = &sensor_inlet_u17_critical_info,
	},
	{
		.name = "inlet_u52",
		.slot_id = FAN_DIR_B2F,
		.correction = -2,
		.lwarn = 45,
		.hwarn = BAD_TEMP,
		.warn_count = 0,
		.flag = CRITICAL_SENSOR_BIT,
		.critical = &sensor_inlet_u52_critical_info,
		.alarm = &sensor_inlet_u52_critical_info,
	},
	{
		.name = "BCM56873_board",
		.slot_id = FAN_DIR_B2F,
		.correction = 15,
		.lwarn = 85,
		.hwarn = 91,
		.warn_count = 0,
		.flag = SWITCH_SENSOR_BIT,
		.critical = &sensor_bcm5870_board_u31_critical_info,
		.alarm = &sensor_bcm5870_board_u31_critical_info,
	},
#ifdef CONFIG_FSC_CONTROL_PID
	{
		.name = "BCM56873_inlet",
		.slot_id = FAN_DIR_B2F,
		.correction = 17,
		.lwarn = 98,
		.hwarn = 104,
		.warn_count = 0,
		.flag = PID_CTRL_BIT,
		.critical = &sensor_bcm5870_inlet_critical_info,
		.alarm = &sensor_bcm5870_inlet_critical_info,
	},
	{
		.name = "cpu_inlet",
		.slot_id = FAN_DIR_B2F,
		.correction = 0,
		.lwarn = 96,
		.hwarn = 100,
		.warn_count = 0,
		.flag = PID_CTRL_BIT,
		.critical = &sensor_cpu_inlet_critical_info,
		.alarm = &sensor_cpu_inlet_critical_info,
	},
	{
		.name = "optical_inlet",
		.slot_id = FAN_DIR_B2F,
		.correction = 0,
		.lwarn = 68,
		.hwarn = 70,
		.warn_count = 0,
		.flag = PID_CTRL_BIT,
		.critical = &sensor_optical_inlet_critical_info,
		.alarm = &sensor_optical_inlet_critical_info,
	},
#endif

	/*F2B*/
	{
		.name = "inlet_u28",
		.slot_id = FAN_DIR_F2B,
		.correction = -6,
		.lwarn = 45,
		.hwarn = BAD_TEMP,
		.warn_count = 0,
		.flag = CRITICAL_SENSOR_BIT,
		.critical = &sensor_inlet_u28_critical_info,
		.alarm = &sensor_inlet_u28_critical_info,
	},
	{
		.name = "inlet_u29",
		.slot_id = FAN_DIR_F2B,
		.correction = -2,
		.lwarn = BAD_TEMP,
		.hwarn = BAD_TEMP,
		.warn_count = 0,
		.flag = 0,
		.critical = &sensor_inlet_u29_critical_info,
		.alarm = &sensor_inlet_u29_critical_info,
	},
	{
		.name = "BCM56873_board",
		.slot_id = FAN_DIR_F2B,
		.correction = 15,
		.lwarn = 85,
		.hwarn = 91,
		.warn_count = 0,
		.flag = SWITCH_SENSOR_BIT,
		.critical = &sensor_bcm5870_board_u30_critical_info,
		.alarm = &sensor_bcm5870_board_u30_critical_info,
	},
#ifdef CONFIG_FSC_CONTROL_PID
	{
		.name = "BCM56873_inlet",
		.slot_id = FAN_DIR_F2B,
		.correction = 17,
		.lwarn = 98,
		.hwarn = 104,
		.warn_count = 0,
		.flag = PID_CTRL_BIT,
		.critical = &sensor_bcm5870_inlet_critical_info,
		.alarm = &sensor_bcm5870_inlet_critical_info,
	},
	{
		.name = "cpu_inlet",
		.slot_id = FAN_DIR_F2B,
		.correction = 0,
		.lwarn = 96,
		.hwarn = 100,
		.warn_count = 0,
		.flag = PID_CTRL_BIT,
		.critical = &sensor_cpu_inlet_critical_info,
		.alarm = &sensor_cpu_inlet_critical_info,
	},
	{
		.name = "optical_inlet",
		.slot_id = FAN_DIR_F2B,
		.correction = 0,
		.lwarn = 68,
		.hwarn = 70,
		.warn_count = 0,
		.flag = PID_CTRL_BIT,
		.critical = &sensor_optical_inlet_critical_info,
		.alarm = &sensor_optical_inlet_critical_info,
	},
#endif
	NULL,
};

static struct fantray_info_stu_sysfs fantray_info[] = {
  {
    .name = "Fantry 1",
    .present = 1,
    .failed = 0,
    .fan1 = fan1_info,
  },
  {
    .name = "Fantry 2",
    .present = 1,
    .failed = 0,
    .fan1 = fan2_info,
  },
  {
    .name = "Fantry 3",
    .present = 1,
    .failed = 0,
    .fan1 = fan3_info,
  },
  {
    .name = "Fantry 4",
    .present = 1,
    .failed = 0,
    .fan1 = fan4_info,
  },
  {
	.name = "PSU 1",
	.present = 1,
	.failed = 0,
	.fan1 = psu1_fan_info,
  },
  {
	.name = "PSU 2",
	.present = 1,
	.failed = 0,
	.fan1 = psu2_fan_info,
  },
  NULL,
};



#define BOARD_INFO_SIZE (sizeof(board_info) \
                        / sizeof(struct board_info_stu_sysfs))
#define FANTRAY_INFO_SIZE (sizeof(fantray_info) \
                        / sizeof(struct fantray_info_stu_sysfs))

struct rpm_to_pct_map rpm_front_map[] = {{20, 4950},
                                         {25, 6150},
                                         {30, 7500},
                                         {35, 8700},
                                         {40, 9900},
                                         {45, 11250},
                                         {50, 12300},
                                         {55, 13650},
                                         {60, 14850},
                                         {65, 16050},
                                         {70, 17400},
                                         {75, 18600},
                                         {80, 19950},
                                         {85, 21000},
                                         {90, 22350},
                                         {95, 23550},
                                         {100, 24150}};

struct rpm_to_pct_map rpm_rear_map[] = {{20, 6000},
                                        {25, 7500},
                                        {30, 8850},
                                        {35, 10500},
                                        {40, 12150},
                                        {45, 13350},
                                        {50, 14850},
                                        {55, 16650},
                                        {60, 18000},
                                        {65, 19350},
                                        {70, 20850},
                                        {75, 22350},
                                        {80, 24000},
                                        {85, 25350},
                                        {90, 26850},
                                        {95, 28350},
                                        {100, 28950}};

struct rpm_to_pct_map psu_rpm_map[] = {{20, 8800},
                                        {25, 8800},
                                        {30, 8800},
                                        {35, 8800},
                                        {40, 8800},
                                        {45, 9920},
                                        {50, 11520},
                                        {55, 13120},
                                        {60, 14560},
                                        {65, 16192},
                                        {70, 17760},
                                        {75, 19296},
                                        {80, 20800},
                                        {85, 21760},
                                        {90, 23424},
                                        {95, 24800},
                                        {100, 26496}};

#define FRONT_MAP_SIZE (sizeof(rpm_front_map) / sizeof(struct rpm_to_pct_map))
#define REAR_MAP_SIZE (sizeof(rpm_rear_map) / sizeof(struct rpm_to_pct_map))
#define PSU_MAP_SIZE (sizeof(psu_rpm_map) / sizeof(struct rpm_to_pct_map))


static struct thermal_policy_t f2b_policy = {
	.level1 = {
		.temp = 0,
		.pwm = 102,
	},
	.level2 = {
		.temp = 20,
		.pwm = 128,
	},
	.level3 = {
		.temp = 25,
		.pwm = 153,
	},
	.level4 = {
		.temp = 30,
		.pwm = 178,
	},
	.level5 = {
		.temp = 35,
		.pwm = 204,
	},
	.level6 = {
		.temp = 40,
		.pwm = 230,
	},
};

static struct thermal_policy_t f2b_one_fail_policy = {
	.level1 = {
		.temp = 0,
		.pwm = 128,
	},
	.level2 = {
		.temp = 20,
		.pwm = 153,
	},
	.level3 = {
		.temp = 25,
		.pwm = 179,
	},
	.level4 = {
		.temp = 30,
		.pwm = 204,
	},
	.level5 = {
		.temp = 35,
		.pwm = 230,
	},
	.level6 = {
		.temp = 40,
		.pwm = 255,
	},
};


static struct thermal_policy_t b2f_policy = {
	.level1 = {
		.temp = 0,
		.pwm = 102,
	},
	.level2 = {
		.temp = 20,
		.pwm = 128,
	},
	.level3 = {
		.temp = 25,
		.pwm = 153,
	},
	.level4 = {
		.temp = 30,
		.pwm = 178,
	},
	.level5 = {
		.temp = 35,
		.pwm = 204,
	},
	.level6 = {
		.temp = 40,
		.pwm = 230,
	},
};

static struct thermal_policy_t b2f_one_fail_policy = {
	.level1 = {
		.temp = 0,
		.pwm = 128,
	},
	.level2 = {
		.temp = 20,
		.pwm = 153,
	},
	.level3 = {
		.temp = 25,
		.pwm = 179,
	},
	.level4 = {
		.temp = 30,
		.pwm = 204,
	},
	.level5 = {
		.temp = 35,
		.pwm = 230,
	},
	.level6 = {
		.temp = 40,
		.pwm = 255,
	},
};


static struct thermal_policy_t pid_f2b_policy = {
	.level1 = {
		.temp = 0,
		.pwm = 102,
	},
	.level2 = {
		.temp = 20,
		.pwm = 128,
	},
	.level3 = {
		.temp = 25,
		.pwm = 153,
	},
	.level4 = {
		.temp = 30,
		.pwm = 178,
	},
	.level5 = {
		.temp = 35,
		.pwm = 204,
	},
	.level6 = {
		.temp = 40,
		.pwm = 230,
	},
};

static struct thermal_policy_t pid_b2f_policy = {
	.level1 = {
		.temp = 0,
		.pwm = 102,
	},
	.level2 = {
		.temp = 20,
		.pwm = 128,
	},
	.level3 = {
		.temp = 25,
		.pwm = 153,
	},
	.level4 = {
		.temp = 30,
		.pwm = 178,
	},
	.level5 = {
		.temp = 35,
		.pwm = 204,
	},
	.level6 = {
		.temp = 40,
		.pwm = 230,
	},
};


static struct thermal_policy_t *policy = NULL;
static int pid_using = 0;
static int direction = 0;


static int write_fan_led(const int fan, const int color);
static int write_fan_speed(const int fan, const int value);
static int write_psu_fan_speed(const int fan, int value);

/*
 * Initialize path cache by writing 0-length string
 */
static int init_path_cache(void)
{
	int i = 0;
	// Temp Sensor datastructure
	for (i = 0; i < BOARD_INFO_SIZE; i++)
	{
		if (board_info[i].alarm != NULL)
			snprintf(board_info[i].alarm->path_cache, PATH_CACHE_SIZE, "");
		if (board_info[i].critical != NULL)
			snprintf(board_info[i].critical->path_cache, PATH_CACHE_SIZE, "");
	}

	return 0;
}

/*
 * Helper function to probe directory, and make full path
 * Will probe directory structure, then make a full path
 * using "<prefix>/hwmon/hwmonxxx/<suffix>"
 * returns < 0, if hwmon directory does not exist or something goes wrong
 */
int assemble_sysfs_path(const char* prefix, const char* suffix,
                        char* full_name, int buffer_size)
{
	int rc = 0;
	int dirname_found = 0;
	char temp_str[PATH_CACHE_SIZE];
	DIR *dir = NULL;
	struct dirent *ent;

	if (full_name == NULL)
		return -1;

	snprintf(temp_str, (buffer_size - 1), "%s/hwmon", prefix);
	dir = opendir(temp_str);
	if (dir == NULL) {
		rc = ENOENT;
		goto close_dir_out;
	}

	while ((ent = readdir(dir)) != NULL) {
		if (strstr(ent->d_name, "hwmon")) {
			// found the correct 'hwmon??' directory
			snprintf(full_name, buffer_size, "%s/%s/%s",
			       temp_str, ent->d_name, suffix);
			dirname_found = 1;
			break;
		}
	}

close_dir_out:
	if (dir != NULL) {
		closedir(dir);
	}

	if (dirname_found == 0) {
		rc = ENOENT;
	}

	return rc;
}

static int adjust_sysnode_path(const char* prefix, const char* suffix,
                        char* full_name, int buffer_size)
{
	int rc = 0;
	FILE *fp;
	int dirname_found = 0;
	char temp_str[PATH_CACHE_SIZE];
	DIR *dir = NULL;
	struct dirent *ent;

	if (full_name == NULL)
		return -1;
	snprintf(temp_str, (buffer_size - 1), "%s/%s", prefix, suffix);
	fp = fopen(temp_str, "r");
	if(fp) {
		fclose(fp);
		return 0;
	}

	/*adjust the path, because the hwmon id may be changed*/
	snprintf(temp_str, (buffer_size - 1), "%s/hwmon", prefix);
	dir = opendir(temp_str);
	if (dir == NULL) {
		rc = ENOENT;
		goto close_dir_out;
	}

	while ((ent = readdir(dir)) != NULL) {
		if (strstr(ent->d_name, "hwmon")) {
			// found the correct 'hwmon??' directory
			snprintf(full_name, buffer_size, "%s/%s/%s",
				   temp_str, ent->d_name, suffix);
			dirname_found = 1;
			break;
		}
	}

close_dir_out:
	if (dir != NULL) {
		closedir(dir);
	}

	if (dirname_found == 0) {
		rc = ENOENT;
	}

	return rc;

}

// Functions for reading from sysfs stub
static int read_sysfs_raw_internal(const char *device, char *value, int log)
{
	FILE *fp;
	int rc, err;

	fp = fopen(device, "r");
	if (!fp) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to open device %s for read: %s",
			     device, strerror(err));
			errno = err;
		}
		return -1;
	}

	rc = fscanf(fp, "%s", value);
	fclose(fp);

	if (rc != 1) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to read device %s: %s",
			     device, strerror(err));
			errno = err;
		}
		return -1;
	}

  return 0;
}

static int read_sysfs_raw(char *sysfs_path, char *buffer)
{
	return read_sysfs_raw_internal(sysfs_path, buffer, 1);
}

// Returns 0 for success, or -1 on failures.
static int read_sysfs_int(char *sysfs_path, int *buffer)
{
	int rc;
	char readBuf[PATH_CACHE_SIZE];

	if (sysfs_path == NULL || buffer == NULL) {
		errno = EINVAL;
		return -1;
	}

	rc = read_sysfs_raw(sysfs_path, readBuf);
	if (rc == 0)
	{
		if (strstr(readBuf, "0x") || strstr(readBuf, "0X"))
			sscanf(readBuf, "%x", buffer);
		else
			sscanf(readBuf, "%d", buffer);
	}
	return rc;
}

static int write_sysfs_raw_internal(const char *device, char *value, int log)
{
	FILE *fp;
	int rc, err;

	fp = fopen(device, "w");
	if (!fp) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to open device %s for write : %s",
			     device, strerror(err));
			errno = err;
		}
		return err;
	}

	rc = fputs(value, fp);
	fclose(fp);

	if (rc < 0) {
		if (log) {
			err = errno;
			syslog(LOG_INFO, "failed to write to device %s", device);
			errno = err;
		}
		return -1;
	}

  return 0;
}

static int write_sysfs_raw(char *sysfs_path, char* buffer)
{
	return write_sysfs_raw_internal(sysfs_path, buffer, 1);
}

// Returns 0 for success, or -1 on failures.
static int write_sysfs_int(char *sysfs_path, int buffer)
{
	int rc;
	char writeBuf[PATH_CACHE_SIZE];

	if (sysfs_path == NULL) {
		errno = EINVAL;
		return -1;
	}

	snprintf(writeBuf, PATH_CACHE_SIZE, "%d", buffer);
	return write_sysfs_raw(sysfs_path, writeBuf);
}

static int read_temp_directly_sysfs(struct sensor_info_sysfs *sensor)
{
	int ret;
	int value;
	char fullpath[PATH_CACHE_SIZE];
	bool use_cache = false;
	int cache_str_len = 0;

	if (sensor == NULL) {
		syslog(LOG_NOTICE, "sensor is null\n");
		return BAD_TEMP;
	}
	// Check if cache is available
	if (sensor->path_cache != NULL) {
		cache_str_len = strlen(sensor->path_cache);
		if (cache_str_len != 0)
			use_cache = true;
	}

	if (use_cache == false) {
		snprintf(fullpath, sizeof(fullpath), "%s/%s", sensor->prefix, sensor->suffix);
		// Update cache, if possible.
		if (sensor->path_cache != NULL)
			snprintf(sensor->path_cache, (PATH_CACHE_SIZE - 1), "%s", fullpath);
		use_cache = true;
	}
	/*
	* By the time control reaches here, use_cache is always true
	* or this function already returned -1. So assume the cache is always on
	*/
	ret = read_sysfs_int(sensor->path_cache, &value);

	/*  Note that Kernel sysfs stub pre-converts raw value in xxxxx format,
	*  which is equivalent to xx.xxx degree - all we need to do is to divide
	*  the read value by 1000
	*/
	if (ret < 0)
		value = ret;
	else
		value = value / 1000;

	if (value < 0) {
		syslog(LOG_ERR, "failed to read temperature bus %s", fullpath);
		return BAD_TEMP;
	}

	usleep(11000);
	return value;
}


static int read_temp_sysfs(struct sensor_info_sysfs *sensor)
{
	int ret;
	int value;
	char fullpath[PATH_CACHE_SIZE];
	bool use_cache = false;
	int cache_str_len = 0;

	if (sensor == NULL) {
		syslog(LOG_NOTICE, "sensor is null\n");
		return BAD_TEMP;
	}
	// Check if cache is available
	if (sensor->path_cache != NULL) {
		cache_str_len = strlen(sensor->path_cache);
		if (cache_str_len != 0)
			use_cache = true;
	}

	if (use_cache == false) {
		// No cached value yet. Calculate the full path first
		ret = assemble_sysfs_path(sensor->prefix, sensor->suffix, fullpath, sizeof(fullpath));
		if(ret != 0) {
			syslog(LOG_NOTICE, "%s: I2C bus %s not available. Failed reading %s\n", __FUNCTION__, sensor->prefix, sensor->suffix);
			return BAD_TEMP;
		}
		// Update cache, if possible.
		if (sensor->path_cache != NULL)
			snprintf(sensor->path_cache, (PATH_CACHE_SIZE - 1), "%s", fullpath);
		use_cache = true;
	}

	/*
	* By the time control reaches here, use_cache is always true
	* or this function already returned -1. So assume the cache is always on
	*/
	ret = read_sysfs_int(sensor->path_cache, &value);

	/*  Note that Kernel sysfs stub pre-converts raw value in xxxxx format,
	*  which is equivalent to xx.xxx degree - all we need to do is to divide
	*  the read value by 1000
	*/
	if (ret < 0)
		value = ret;
	else
		value = value / 1000;

	if (value < 0) {
		syslog(LOG_ERR, "failed to read temperature bus %s", fullpath);
		return BAD_TEMP;
	}

	usleep(11000);
	return value;
}


static int read_critical_max_temp(void)
{
	int i;
	int temp, max_temp = 0;
#ifdef FANCTRL_SIMULATION
	static int t = 50;
	static int div = -2;
#endif

	struct board_info_stu_sysfs *info;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		info = &board_info[i];
		if(info->slot_id != direction)
			continue;
		if(info->critical && (info->flag & CRITICAL_SENSOR_BIT)) {
			temp = info->critical->read_sysfs(info->critical);
			if(temp != -1) {
				temp += info->correction;
				info->critical->temp = temp;
				if(info->critical->t1 == 0)
					info->critical->t1 = temp;
				if(info->critical->t2 == 0)
					info->critical->t2 = temp;
				if(temp > max_temp)
					max_temp = temp;
			}
		}
	}
#ifdef FANCTRL_SIMULATION
	if(t <= 15) {
		div = 2;
	} else if(t >= 50) {
		div = -2;
	}
	t += div;
	max_temp = t;
#endif
	syslog(LOG_DEBUG, "[zmzhan]%s: critical: max_temp=%d", __func__, max_temp);

	return max_temp;
}

static int read_pid_max_temp(void)
{
	int i;
	int temp, max_temp = 0;
	struct board_info_stu_sysfs *info;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		info = &board_info[i];
		if(info->slot_id != direction)
			continue;
		if(info->critical && (info->flag & PID_CTRL_BIT)) {
			temp = info->critical->read_sysfs(info->critical);
			if(temp != -1) {
				temp += info->correction;
				info->critical->temp = temp;
				if(info->critical->t1 == 0)
					info->critical->t1 = temp;
				if(info->critical->t2 == 0)
					info->critical->t2 = temp;
				if(temp > max_temp)
					max_temp = temp;
			}
			syslog(LOG_DEBUG, "[zmzhan]%s: %s: temp=%d", __func__, info->name, temp);
		}
	}

	return max_temp;
}

static int calculate_pid_pwm(int fan_pwm)
{
	int i;
	int pwm, max_pwm = 0;
	struct board_info_stu_sysfs *info;
	struct sensor_info_sysfs *critical;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		info = &board_info[i];
		if(info->slot_id != direction)
			continue;
		if(info->critical && (info->flag & PID_CTRL_BIT)) {
			critical = info->critical;
			if(critical->old_pwm == 0)
				critical->old_pwm = fan_pwm;
			pwm = critical->old_pwm + critical->p * (critical->temp - critical->t1 + 
				critical->i * (critical->temp - critical->setpoint)) + critical->d *
				(critical->temp - 2 * critical->t1 + critical->t2);
			if(pwm < critical->min_output)
				pwm = critical->min_output;
			if(pwm > max_pwm)
				max_pwm = pwm;

			syslog(LOG_DEBUG, "[zmzhan]%s: %s: pwm=%d, old_pwm=%d, p=%f, i=%f, d=%f, setporint=%f \
				temp=%d, t1=%d, t2=%d", __func__, info->name, pwm, critical->old_pwm, critical->p,
				critical->i, critical->d, critical->setpoint, critical->temp, critical->t1, critical->t2);
		}
	}

	return max_pwm;
}


static int alarm_temp_update(int *alarm)
{
	int i, fan;
	int temp, max_temp = 0;
	struct board_info_stu_sysfs *info;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		info = &board_info[i];
		if(info->slot_id != direction)
			continue;
		if(info->alarm) {
			temp = info->alarm->read_sysfs(info->alarm);
			if(temp != -1) {
				temp += info->correction;
				info->alarm->temp = temp;
				if(info->hwarn != BAD_TEMP && 
					(temp >= info->hwarn || ((info->hwarn - temp <= ALARM_TEMP_THRESHOLD) && info->warn_count))) {
					if(++info->warn_count >= ALARM_START_REPORT) {
						syslog(LOG_WARNING, "Warning: %s arrived %d C(High Warning: >= %d C)",
							info->name, temp, info->hwarn);
						info->warn_count = 0;
						for (fan = 0; fan < TOTAL_FANS; fan++) {
							write_fan_speed(fan, FAN_MAX);
						}
						write_psu_fan_speed(fan, FAN_MAX);
						info->flag |= HIGH_WARN_BIT;
						*alarm |= HIGH_WARN_BIT;
					}
				} else if(info->lwarn != BAD_TEMP && 
						(temp >= info->lwarn || ((info->lwarn - temp <= ALARM_TEMP_THRESHOLD) && info->warn_count))) {
					if(++info->warn_count >= ALARM_START_REPORT) {
						syslog(LOG_WARNING, "Warning: %s arrived %d C(Low Warning: >= %d C)",
							info->name, temp, info->lwarn);
						info->warn_count = 0;
						info->flag |= LOW_WARN_BIT;
						*alarm |= LOW_WARN_BIT;
					}
				} else {
					if(info->flag & HIGH_WARN_BIT) {
						syslog(LOG_INFO, "Recovery: %s arrived %d C", info->name, temp);
						info->flag &= ~HIGH_WARN_BIT;
						*alarm &= ~HIGH_WARN_BIT;
					} else if(info->flag & LOW_WARN_BIT) {
						syslog(LOG_INFO, "Recovery: %s arrived %d C", info->name, temp);
						if(++info->warn_count >= WARN_RECOVERY_COUNT) {
							info->flag &= ~LOW_WARN_BIT;
							*alarm &= ~LOW_WARN_BIT;
						}
					}
				}
			}
		}
	}
	//printf("%s: alarm: max_temp=%d\n", __func__, max_temp);

	return max_temp;
}

static int calculate_raising_fan_pwm(int temp)
{
	int val;

	if(temp >= policy->level6.temp) {
		val = policy->level6.pwm;
	} else if(temp >= policy->level5.temp) {
		val = policy->level5.pwm;
	} else if(temp >= policy->level4.temp) {
		val = policy->level4.pwm;
	} else if(temp >= policy->level3.temp) {
		val = policy->level3.pwm;
	} else if(temp >= policy->level2.temp) {
		val = policy->level2.pwm;
	} else if(temp >= policy->level1.temp) {
		val = policy->level1.pwm;
	} else {
		val = policy->level1.pwm;
	}

	syslog(LOG_DEBUG, "[zmzhan]%s: pwm=%d", __func__, val);

	return val;
}
static int calculate_falling_fan_pwm(int temp)
{
	int val;
	
	if(temp >= policy->level6.temp - CRITICAL_TEMP_HYST) {
		val = policy->level6.pwm;
	}else if(temp >= policy->level5.temp - CRITICAL_TEMP_HYST) {
		val = policy->level5.pwm;
	} else if(temp >= policy->level4.temp - CRITICAL_TEMP_HYST) {
		val = policy->level4.pwm;
	} else if(temp >= policy->level3.temp - CRITICAL_TEMP_HYST) {
		val = policy->level3.pwm;
	} else if(temp >= policy->level2.temp - CRITICAL_TEMP_HYST) {
		val = policy->level2.pwm;
	} else if(temp >= policy->level1.temp - CRITICAL_TEMP_HYST) {
		val = policy->level1.pwm;
	} else {
		val = policy->level1.pwm;
	}
	syslog(LOG_DEBUG, "[zmzhan]%s: pwm=%d", __func__, val);

	return val;
}

#define PSU_FAN_LOW 45
static int calculate_psu_raising_fan_pwm(int temp)
{
	int slope;
	int val;

	if(temp < RAISING_TEMP_LOW) {
		return PSU_FAN_LOW;
	} else if(temp >= RAISING_TEMP_LOW && temp < RAISING_TEMP_HIGH) {
		slope = (FAN_HIGH - PSU_FAN_LOW) / (RAISING_TEMP_HIGH - RAISING_TEMP_LOW);
		val = PSU_FAN_LOW + slope * temp;
		return val;
	} else  {
		return FAN_HIGH;
	}
		return FAN_HIGH;
}
static int calculate_psu_falling_fan_pwm(int temp)
{
	int slope;
	int val;

	if(temp < FALLING_TEMP_LOW) {
		return PSU_FAN_LOW;
	} else if(temp >= FALLING_TEMP_LOW && temp < FALLING_TEMP_HIGH) {
		slope = (FAN_HIGH - PSU_FAN_LOW) / (FALLING_TEMP_HIGH - FALLING_TEMP_LOW);
		val = PSU_FAN_LOW + slope * temp;
		return val;		
	} else  {
		return FAN_HIGH;
	}

	return FAN_HIGH;
}


/*
 * Fan number here is 0-based
 * Note that 1 means present
 */
static int fan_is_present_sysfs(int fan, struct fan_info_stu_sysfs *fan_info)
{
	int ret;
	char buf[PATH_CACHE_SIZE];
	int rc = 0;

	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->fan_present_prefix);

	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read module present %s node", fan_info->fan_present_prefix);
		return -1;
	}

	usleep(11000);

	if (ret != 0) {
		if(fan < TOTAL_FANS)
			syslog(LOG_ERR, "%s: Fantray-%d not present", __func__, fan + 1);
		else
			syslog(LOG_ERR, "%s: PSU-%d not present", __func__, fan - TOTAL_FANS + 1);
		return 0;
	} else {
		return 1;
	}

	return 0;
}


// Note that the fan number here is 0-based
static int set_fan_sysfs(int fan, int value)
{
	int ret;
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;

	fantray = &fantray_info[fan];
	fan_info = &fantray->fan1;
	
	char fullpath[PATH_CACHE_SIZE];

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray->present = 0; //not preset
		return -1;
	} else if(ret == 1) {
		fantray->present = 1;
	} else {
		return -1;
	}

	snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->pwm_prefix);
	adjust_sysnode_path(fan_info->prefix, fan_info->pwm_prefix, fullpath, sizeof(fullpath));
	ret = write_sysfs_int(fullpath, value);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s/%s, value %#x",
		fan_info->prefix, fan_info->pwm_prefix, value);
		return -1;
	}
	usleep(11000);

	return 0;
}

static int write_fan_led_sysfs(int fan, const int color)
{
	int ret;
	char fullpath[PATH_CACHE_SIZE];
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;
	
	fantray = &fantray_info[fan];
	fan_info = &fantray->fan1;
	

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray->present = 0; //not preset
		return -1;
	} else if(ret == 1) {
		fantray->present = 1;
	} else {
		return -1;
	}

	snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->fan_led_prefix);
	ret = write_sysfs_int(fullpath, color);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s/%s, value %#x",
		fan_info->prefix, fan_info->fan_led_prefix, color);
		return -1;
	}
	usleep(11000);

	return 0;
}


/* Set fan speed as a percentage */
static int write_fan_speed(const int fan, const int value)
{
 	return set_fan_sysfs(fan, value);
}

/* Set up fan LEDs */
static int write_fan_led(const int fan, const int color)
{
	if(fan >= TOTAL_FANS)
		return 0; //not support write PSU LED
	return write_fan_led_sysfs(fan, color);
}

static int get_psu_pwm(void)
{
	int i;
	int ret;
	int pwm = 0, tmp = 0;
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;

	for(i = TOTAL_FANS; i < TOTAL_FANS + TOTAL_PSUS; i++) {
		fantray = &fantray_info[i];
		fan_info = &fantray->fan1;
		
		char fullpath[PATH_CACHE_SIZE];
		
		ret = fan_is_present_sysfs(i, fan_info);
		if(ret == 0) {
			fantray->present = 0; //not preset
			continue;
		} else if(ret == 1) {
			fantray->present = 1;
		} else {
			continue;
		}
		
		snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->rear_fan_prefix, fan_info->pwm_prefix);
		adjust_sysnode_path(fan_info->rear_fan_prefix, fan_info->pwm_prefix, fullpath, sizeof(fullpath));
		read_sysfs_int(fullpath, &tmp);
		if(tmp > 100)
			tmp = 0;
		if(tmp > pwm)
			pwm = tmp;
		usleep(11000);
	}

	pwm = pwm * 255 / 100;

	return pwm;
}
/* Set PSU fan speed as a percentage */
static int write_psu_fan_speed(const int fan, int value)
{
	int i;
	int ret;
	char fullpath[PATH_CACHE_SIZE];
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;


	value = value * 100 /255; //convert it to pct
	for(i = TOTAL_FANS; i < TOTAL_FANS + TOTAL_PSUS; i++) {
		fantray = &fantray_info[i];
		fan_info = &fantray->fan1;
		
		ret = fan_is_present_sysfs(i, fan_info);
		if(ret == 0) {
			fantray->present = 0; //not preset
			continue;
		} else if(ret == 1) {
			fantray->present = 1;
		} else {
			continue;
		}
		snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->rear_fan_prefix, PSU_SPEED_CTRL_NODE);
		adjust_sysnode_path(fan_info->rear_fan_prefix, PSU_SPEED_CTRL_NODE, fullpath, sizeof(fullpath));
		ret = write_sysfs_int(fullpath, PSU_SPEED_CTRL_ENABLE);
		if(ret < 0) {
			syslog(LOG_ERR, "failed to enable control PSU speed");
		}

		snprintf(fullpath, PATH_CACHE_SIZE, "%s/%s", fan_info->rear_fan_prefix, fan_info->pwm_prefix);
		adjust_sysnode_path(fan_info->rear_fan_prefix, fan_info->pwm_prefix, fullpath, sizeof(fullpath));
		ret = write_sysfs_int(fullpath, value);
		if(ret < 0) {
			syslog(LOG_ERR, "failed to set fan %s/%s, value %#x",
			fan_info->prefix, fan_info->pwm_prefix, value);
			continue;
		}
		usleep(11000);
	}

	return 0;
}

/* Set up fan LEDs */
static int write_psu_fan_led(const int fan, const int color)
{
	int err;
	
	err = write_fan_led_sysfs(TOTAL_FANS, color);
	err += write_fan_led_sysfs(TOTAL_FANS, color);

	return err;
}


static int fan_rpm_to_pct(const struct rpm_to_pct_map *table, const int table_len, int rpm)
{
	int i;
	
	for (i = 0; i < table_len; i++) {
		if (table[i].rpm > rpm) {
			break;
		}
	}

	/*
	 * If the fan RPM is lower than the lowest value in the table,
	 * we may have a problem -- fans can only go so slow, and it might
	 * have stopped.  In this case, we'll return an interpolated
	 * percentage, as just returning zero is even more problematic.
	 */

	if (i == 0) {
		return (rpm * table[i].pct) / table[i].rpm;
	} else if (i == table_len) { // Fell off the top?
		return table[i - 1].pct;
	}
	
	// Interpolate the right percentage value:
	
	int percent_diff = table[i].pct - table[i - 1].pct;
	int rpm_diff = table[i].rpm - table[i - 1].rpm;
	int fan_diff = table[i].rpm - rpm;

	return table[i].pct - (fan_diff * percent_diff / rpm_diff);
}


/*return: 1 OK, 0 not OK*/
int fan_speed_okay(const int fan, int speed, const int slop)
{
	int ret;
	char buf[PATH_CACHE_SIZE];
	int rc = 1;
	int front_speed, front_pct;
	int rear_speed, rear_pct;
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;
	
	fantray = &fantray_info[fan];
	fan_info = &fantray->fan1;

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray->present = 0; //not preset
		return 0;
	} else if(ret == 1) {
		fantray->present = 1;
	}

	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->front_fan_prefix);

	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read %s node", fan_info->front_fan_prefix);
		return -1;
	}
	front_speed = ret;
	usleep(11000);
	if(front_speed < FAN_FAIL_RPM) {
		if(fan_info->front_failed++ >= FAN_FAIL_COUNT) {
			fan_info->front_failed = FAN_FAIL_COUNT;
			syslog(LOG_WARNING, "%s front speed %d, less than %d ", fantray->name, front_speed, FAN_FAIL_RPM);
		}
	} else if(speed == FAN_MAX && (front_speed < (FAN_FRONTT_SPEED_MAX * (100 - slop) / 100))){
		if(fan_info->front_failed++ >= FAN_FAIL_COUNT) {
			fan_info->front_failed = FAN_FAIL_COUNT;
			syslog(LOG_WARNING, "%s front speed %d, less than %d%% of max speed(%d)", 
				fantray->name, front_speed, 100 - slop, speed);
		}
	} else {
		fan_info->front_failed = 0;
	}

	memset(buf, 0, PATH_CACHE_SIZE);
	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->prefix, fan_info->rear_fan_prefix);

	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read %s node", fan_info->front_fan_prefix);
		return -1;
	}
	rear_speed = ret;
	if(rear_speed < FAN_FAIL_RPM) {
		if(fan_info->rear_failed++ >= FAN_FAIL_COUNT) {
			fan_info->rear_failed = FAN_FAIL_COUNT;
			syslog(LOG_WARNING, "%s rear speed %d, less than %d ", fantray->name, rear_speed, FAN_FAIL_RPM);
		}
	} else if(speed == FAN_MAX && (rear_speed < (FAN_REAR_SPEED_MAX * (100 - slop) / 100))){
		if(fan_info->rear_failed++ >= FAN_FAIL_COUNT) {
			fan_info->rear_failed = FAN_FAIL_COUNT;
			syslog(LOG_WARNING, "%s rear speed %d, less than %d%% of max speed(%d)", 
				fantray->name, rear_speed, 100 - slop, speed);
		}
	} else {
		fan_info->rear_failed = 0;
	}

	if(fan_info->front_failed >= FAN_FAIL_COUNT && fan_info->rear_failed >= FAN_FAIL_COUNT) {
		fan_info->front_failed = 0;
		fan_info->rear_failed = 0;
		fantray->failed = 1;
	}
	//syslog(LOG_DEBUG, "[zmzhan]%s: front_speed = %d, rear_speed = %d", __func__, front_speed, rear_speed);
	if(fan_info->front_failed >= FAN_FAIL_COUNT || fan_info->rear_failed >= FAN_FAIL_COUNT) {
		 //syslog(LOG_WARNING, "%s front %d, rear %d, expected %d",
		 //	fantray->name, front_speed, rear_speed, speed);
		 return 0;
	}

	return 1;

}

/*return: 1 OK, 0 not OK*/
int psu_speed_okay(const int fan, int speed, const int slop)
{
	int ret;
	char buf[PATH_CACHE_SIZE];
	int rc = 0;
	int psu_speed, pct;
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;
	
	fantray = &fantray_info[fan];
	fan_info = &fantray->fan1;

	ret = fan_is_present_sysfs(fan, fan_info);
	if(ret == 0) {
		fantray->present = 0; //not preset
		return 0;
	} else if(ret == 1) {
		fantray->present = 1;
	}

	snprintf(buf, PATH_CACHE_SIZE, "%s/%s", fan_info->rear_fan_prefix, fan_info->front_fan_prefix);
	adjust_sysnode_path(fan_info->rear_fan_prefix, fan_info->front_fan_prefix, buf, sizeof(buf));
	rc = read_sysfs_int(buf, &ret);
	if(rc < 0) {
		syslog(LOG_ERR, "failed to read %s node", fan_info->front_fan_prefix);
		return -1;
	}
	psu_speed = ret;
	usleep(11000);
	if(psu_speed < FAN_FAIL_RPM) {
		if(fan_info->front_failed++ >= FAN_FAIL_COUNT) {
			syslog(LOG_WARNING, "%s speed %d, less than %d ", fantray->name, psu_speed, FAN_FAIL_RPM);
		}
	} else if(speed == FAN_MAX && (psu_speed < (PSU_SPEED_MAX * (100 - slop) / 100))){
		if(fan_info->front_failed++ >= FAN_FAIL_COUNT) {
			syslog(LOG_WARNING, "%s speed %d, less than %d%% of max speed(%d)", 
				fantray->name, psu_speed, 100 - slop, speed);
		}
	} else {
		fan_info->front_failed = 0;
	}

	if(fan_info->front_failed >= FAN_FAIL_COUNT)
		return 0;

	return 1;

}

static int fancpld_watchdog_enable(void)
{
	int ret;
	char fullpath[PATH_CACHE_SIZE];
	
	snprintf(fullpath, PATH_CACHE_SIZE, "%s", FAN_WDT_ENABLE_SYSFS);
	ret = write_sysfs_int(fullpath, 1);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s, value 1",
		FAN_WDT_ENABLE_SYSFS);
		return -1;
	}
	usleep(11000);
	snprintf(fullpath, PATH_CACHE_SIZE, "%s", FAN_WDT_TIME_SYSFS);
	ret = write_sysfs_int(fullpath, FAN_WDT_TIME);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set fan %s, value 1",
		FAN_WDT_TIME_SYSFS);
		return -1;
	}
	usleep(11000);

	return 0;
}

static int system_shutdown(const char *why)
{
	int ret;

	syslog(LOG_EMERG, "Shutting down:  %s", why);

	ret = write_sysfs_int(PSU1_SHUTDOWN_SYSFS, 1);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set PSU1 shutdown");
		return -1;
	}
	ret = write_sysfs_int(PSU2_SHUTDOWN_SYSFS, 1);
	if(ret < 0) {
		syslog(LOG_ERR, "failed to set PSU2 shutdown");
		return -1;
	}

	stop_watchdog();

	sleep(2);
	exit(2);

	return 0;
}

static int get_fan_direction(void)
{
	/*add the code in later, now using default value F2B*/


	//return FAN_DIR_F2B;
	return FAN_DIR_B2F;
}

static int pid_inlet_control_parser(struct thermal_policy_t *policy, FILE *fp)
{
	char *p;
	char buf[PID_FILE_LINE_MAX];

	while(fgets(buf, PID_FILE_LINE_MAX, fp) != NULL) {
		if(buf[0] == '#' || strlen(buf) <= 0 || buf[0] == '\r' || buf[0] == '\n')
			continue;

		p = strtok(buf, "=");
		while(p != NULL) {
			if(!strncmp(p, "level1_temp", strlen("level1_temp"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level1.temp = atoi(p);
			} else if(!strncmp(p, "level2_temp", strlen("level2_temp"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level2.temp = atoi(p);
			} else if(!strncmp(p, "level3_temp", strlen("level3_temp"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level3.temp = atoi(p);
			} else if(!strncmp(p, "level4_temp", strlen("level4_temp"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level4.temp = atoi(p);
			} else if(!strncmp(p, "level5_temp", strlen("level5_temp"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level5.temp = atoi(p);
			} else if(!strncmp(p, "level1_pwm", strlen("level1_pwm"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level1.pwm = atoi(p);
			} else if(!strncmp(p, "level2_pwm", strlen("level2_pwm"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level2.pwm = atoi(p);
			} else if(!strncmp(p, "level3_pwm", strlen("level3_pwm"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level3.pwm = atoi(p);
			} else if(!strncmp(p, "level4_pwm", strlen("level4_pwm"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level4.pwm = atoi(p);
			} else if(!strncmp(p, "level5_pwm", strlen("level5_pwm"))) {
				p = strtok(NULL, "=");
				if(p)
					policy->level5.pwm = atoi(p);
				syslog(LOG_DEBUG, "%s: level1_temp=%d, level2_temp=%d, level3_temp=%d, level4_temp=%d, level5_temp=%d, \
					level1_pwm = %d, level2_pwm=%d, level3_pwm=%d, level4_pwm=%d, level5_pwm=%d", __func__,
					policy->level1.temp, policy->level2.temp, policy->level3.temp, policy->level4.temp, policy->level5.temp,
					policy->level1.pwm, policy->level2.pwm, policy->level3.pwm, policy->level4.pwm, policy->level5.pwm);
				return 0;
			}
			
			p = strtok(NULL, "=");
		}
	}
	
	return 0;
}

static int pid_ini_parser(struct board_info_stu_sysfs *info, FILE *fp)
{
	char *p;
	char buf[PID_FILE_LINE_MAX];
	struct sensor_info_sysfs *sensor;
	sensor = info->critical;

	while(fgets(buf, PID_FILE_LINE_MAX, fp) != NULL) {
		if(buf[0] == '#' || strlen(buf) <= 0 || buf[0] == '\r' || buf[0] == '\n')
			continue;

		p = strtok(buf, "=");
		while(p != NULL) {
			if(!strncmp(p, "PID_enable", strlen("PID_enable"))) {
				p = strtok(NULL, "=");
				if(p) {
					pid_using = atoi(p);
				} else {
					pid_using = 0;
				}
				return 0;
			} else if(!strncmp(p, "setpoint", strlen("setpoint"))) {
				p = strtok(NULL, "=");
				if(p)
					sensor->setpoint = atof(p);
			} else if(!strncmp(p, "P", strlen("P"))) {
				p = strtok(NULL, "=");
				if(p)
					sensor->p = atof(p);
			} else if(!strncmp(p, "I", strlen("I"))) {
				p = strtok(NULL, "=");
				if(p)
					sensor->i = atof(p);
			} else if(!strncmp(p, "D", strlen("D"))) {
				p = strtok(NULL, "=");
				if(p)
					sensor->d = atof(p);
			} else if(!strncmp(p, "min_output", strlen("min_output"))) {
				p = strtok(NULL, "=");
				if(p)
					sensor->min_output = atof(p);
				syslog(LOG_DEBUG, "%s: setpoint=%f, P=%f, I=%f, D=%f, min_output=%f",
					__func__, sensor->setpoint, sensor->p, sensor->i, sensor->d, sensor->min_output);
				return 0;
			}
			p = strtok(NULL, "=");
		}
	}
	
	return 0;
}

static int load_pid_config(struct thermal_policy_t *policy)
{
	int i;
	FILE *fp;
	char buf[PID_FILE_LINE_MAX];
	char *p;
	int len;
	struct board_info_stu_sysfs *binfo = &board_info[0];

	fp = fopen(PID_CONFIG_PATH, "r");
	if(!fp) {
		pid_using = 0;
		syslog(LOG_NOTICE, "PID configure file does not find, not using PID");
		return 0;
	}
	while(fgets(buf, PID_FILE_LINE_MAX, fp) != NULL) {
		len = strlen(buf);
		buf[len - 1] = '\0';
		if(buf[0] == '#' || strlen(buf) <= 0 || buf[0] == '\r' || buf[0] == '\n')
			continue;
		p = strtok(buf, "[");
		while(p != NULL) {
			if(!strncmp(p, "PID enable", strlen("PID enable"))) {
				pid_ini_parser(binfo, fp);
			} else if(!strncmp(p, "Inlet control", strlen("Inlet control"))) {
				pid_inlet_control_parser(policy, fp);
			} else {
				for(i = 0; i < BOARD_INFO_SIZE; i++) {
					binfo = &board_info[i];
					if(!strncmp(binfo->name, p, strlen(binfo->name))) {
						pid_ini_parser(binfo, fp);
						break;
					}
				}
			}
			p = strtok(NULL, "[");
		}
	}

	fclose(fp);
	return 0;
}

static int policy_init(void)
{
	int slope;
	struct thermal_policy_t *tmp;

	direction = get_fan_direction();
	if(direction == FAN_DIR_B2F) {
		policy = &b2f_policy;
		tmp = &pid_b2f_policy;
	} else {
		policy = &f2b_policy;
		tmp = &pid_f2b_policy;
	}

	load_pid_config(tmp);
	if(pid_using == 0) {
		syslog(LOG_NOTICE, "PID configure: not using PID");
		//return 0;
	}

	return 0;
}
static int get_switch_pwm(int old_pwm)
{
	int i;
	int pwm = 0;
	int temp;
	struct board_info_stu_sysfs *binfo;
	struct sensor_info_sysfs *sensor;

	for(i = 0; i < BOARD_INFO_SIZE; i++) {
		binfo = &board_info[i];
		if((binfo->slot_id == direction) && (!strcmp(binfo->name, "BCM56873_inlet"))) {
			sensor = binfo->critical;
			if(!sensor)
				sensor = binfo->alarm;
			if(!sensor)
				return -1;
			temp = sensor->read_sysfs(sensor);
			if(temp != -1) {
				temp += binfo->correction;
				sensor->temp = temp;
			}
			if(sensor->t1 == 0)
				sensor->t1 = sensor->temp;
			if(sensor->t2 == 0)
				sensor->t2 = sensor->temp;
			
			if(sensor->old_pwm == 0)
				sensor->old_pwm = old_pwm;

			if(temp < 95)
				pwm = 0;
			else
				pwm = sensor->old_pwm + 2 * (sensor->temp - sensor->t1) + 1 * (sensor->temp - 95);
			syslog(LOG_DEBUG, "[zmzhan]%s: switch pwm=%d(old_pwm=%d, temp=%d, t1=%d)", __func__, pwm, sensor->old_pwm, sensor->temp, sensor->t1);
			if(pwm < 0)
				pwm = 0;
			sensor->old_pwm = pwm;
			sensor->t2 = sensor->t1;
			sensor->t1 = sensor->temp;
			break;
		}
	}

	return pwm;
}

int main(int argc, char **argv) {
	int critical_temp;
	int old_temp = -1;
	int raising_pwm;
	int falling_pwm;
	struct fantray_info_stu_sysfs *info;
	int fan_speed = 102;
	int bad_reads = 0;
	int fan_failure = 0;
	int sub_failed = 0;
	int one_failed = 0; //recored one system fantray failed
	int old_speed = FAN_MEDIUM;
	int fan_bad[TOTAL_FANS + TOTAL_PSUS] = {0};
	int fan;
	unsigned log_count = 0; // How many times have we logged our temps?
	int prev_fans_bad = 0;
	int shutdown_delay = 0;
	int psu_pwm;
	int switch_pwm = 0;
	int pid_pwm = 0;
	int alarm = 0;
#ifdef CONFIG_PSU_FAN_CONTROL_INDEPENDENT
	int psu_old_temp = 0;
	int psu_raising_pwm;
	int psu_falling_pwm;
	int psu_fan_speed = FAN_MEDIUM;
#endif
	struct fantray_info_stu_sysfs *fantray;
	struct fan_info_stu_sysfs *fan_info;

	// Initialize path cache
	init_path_cache();

	// Start writing to syslog as early as possible for diag purposes.
	openlog("fand", LOG_CONS, LOG_DAEMON);

	daemon(1, 0);

	syslog(LOG_DEBUG, "Starting up;  system should have %d fans.", TOTAL_FANS);

	/* Start watchdog in manual mode */
	//start_watchdog(0); //zmzhan remove temp

	/* Set watchdog to persistent mode so timer expiry will happen independent
	* of this process's liveliness. */
	//set_persistent_watchdog(WATCHDOG_SET_PERSISTENT);

	fancpld_watchdog_enable();

	policy_init();

	sleep(5);  /* Give the fans time to come up to speed */

	while (1) {
		/* Read sensors */
		critical_temp = read_critical_max_temp();
		alarm_temp_update(&alarm);

		if (critical_temp == BAD_TEMP) {
			if(bad_reads++ >= 10) {
				if(critical_temp == BAD_TEMP) {
					syslog(LOG_ERR, "Critical Temp read error!");
				}
				bad_reads = 0;
			}
		}

#if 0
		/* Protection heuristics */
		if(critical_temp > SYSTEM_LIMIT) {
			for (fan = 0; fan < TOTAL_FANS; fan++) {
				write_fan_speed(fan, fan_speed);
			}
			if(shutdown_delay++ > SHUTDOWN_DELAY_TIME)
				system_shutdown("Critical temp limit reached");
		}
#endif
		/*
		 * Calculate change needed -- we should eventually
		 * do something more sophisticated, like PID.
		 *
		 * We should use the intake temperature to adjust this
		 * as well.
		 */

		/* Other systems use a simpler built-in table to determine fan speed. */
		raising_pwm = calculate_raising_fan_pwm(critical_temp);
		falling_pwm = calculate_falling_fan_pwm(critical_temp);
		policy->old_pwm = fan_speed;
		if(old_temp <= critical_temp) {
			/*raising*/
			if(raising_pwm >= fan_speed) {
				fan_speed = raising_pwm;
			}
		} else {
			/*falling*/
			if(falling_pwm <= fan_speed ) {
				fan_speed = falling_pwm;
			}
		}
#ifndef CONFIG_PSU_FAN_CONTROL_INDEPENDENT
		psu_pwm = get_psu_pwm();
		//if(fan_speed < psu_pwm)
		//	fan_speed = psu_pwm;
		if(pid_using == 0) {
			switch_pwm = get_switch_pwm(fan_speed);
			if(fan_speed < switch_pwm)
				fan_speed = switch_pwm;
		}
#endif
		if(pid_using) {
			read_pid_max_temp();
			pid_pwm = calculate_pid_pwm(fan_speed);
			if(pid_pwm > fan_speed)
				fan_speed = pid_pwm;
		}
		syslog(LOG_DEBUG, "[zmzhan]%s: fan_speed=%d, psu_pwm=%d, pid_pwm=%d, switch_pwm=%d", 
			__func__, fan_speed, psu_pwm, pid_pwm, switch_pwm);
		policy->pwm = fan_speed;
		old_temp = critical_temp;
#ifdef CONFIG_PSU_FAN_CONTROL_INDEPENDENT
		psu_raising_pwm = calculate_psu_raising_fan_pwm(critical_temp);
		psu_falling_pwm = calculate_psu_falling_fan_pwm(critical_temp);
		if(psu_old_temp <= critical_temp) {
			/*raising*/
			if(psu_raising_pwm >= psu_fan_speed) {
				psu_fan_speed = psu_raising_pwm;
			}
		} else {
			/*falling*/
			if(psu_falling_pwm <= psu_fan_speed ) {
				psu_fan_speed = psu_falling_pwm;
			}
		}
		psu_old_temp = critical_temp;
#endif

		/*
		 * Update fans only if there are no failed ones. If any fans failed
		 * earlier, all remaining fans should continue to run at max speed.
		 */
		if (fan_failure == 0) {
			if (log_count++ % REPORT_TEMP == 0) {
				syslog(LOG_NOTICE, "critical temp %d, fan speed %d%%",
					critical_temp, fan_speed * 100 / FAN_MAX);
				syslog(LOG_NOTICE, "Fan speed changing from %d%% to %d%%",
					old_speed * 100 / FAN_MAX, fan_speed * 100 / FAN_MAX);
			}
			for (fan = 0; fan < TOTAL_FANS; fan++) {
				if((alarm & HIGH_WARN_BIT) == 0)
					write_fan_speed(fan, fan_speed);
			}
#ifdef CONFIG_PSU_FAN_CONTROL_INDEPENDENT
			write_psu_fan_speed(fan, psu_fan_speed);
#else
			if((alarm & HIGH_WARN_BIT) == 0)
				write_psu_fan_speed(fan, fan_speed);
#endif
		}

		
		/*
		 * Wait for some change.  Typical I2C temperature sensors
		 * only provide a new value every second and a half, so
		 * checking again more quickly than that is a waste.
		 *
		 * We also have to wait for the fan changes to take effect
		 * before measuring them.
		 */

		sleep(3);

	    /* Check fan RPMs */

	    for (fan = 0; fan < TOTAL_FANS; fan++) {
			/*
			* Make sure that we're within some percentage
			* of the requested speed.
			*/
			if (fan_speed_okay(fan, fan_speed, FAN_FAILURE_OFFSET)) {
				if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
					write_fan_led(fan, FAN_LED_GREEN);
					syslog(LOG_CRIT, "Fan %d has recovered", fan + 1);
				}
				fan_bad[fan] = 0;
			} else {
				fan_bad[fan]++;
			}
		}
		for(fan = TOTAL_FANS; fan < TOTAL_FANS + TOTAL_PSUS; fan++) {
			if (psu_speed_okay(fan, fan_speed, FAN_FAILURE_OFFSET)) {
				if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
					//write_fan_led(fan, FAN_LED_GREEN);
					syslog(LOG_CRIT, "PSU %d has recovered", fan - TOTAL_FANS + 1);
				}
				fan_bad[fan] = 0;
			} else {
				fan_bad[fan]++;
			}
		}

		fan_failure = 0;
		sub_failed = 0;
		one_failed = 0;
		for (fan = 0; fan < TOTAL_FANS + TOTAL_PSUS; fan++) {
			if (fan_bad[fan] >= FAN_FAILURE_THRESHOLD) {
				fantray = &fantray_info[fan];
				fan_info = &fantray->fan1;
				if(fan_info->front_failed >= FAN_FAIL_COUNT) {
					sub_failed++;
					one_failed++;
					syslog(LOG_DEBUG, "[zmzhan]%s:fan[%d] front_failed=%d", __func__, fan, fan_info->front_failed);
				}
				if(fan_info->rear_failed >= FAN_FAIL_COUNT) {
					sub_failed++;
					one_failed++;
					syslog(LOG_DEBUG, "[zmzhan]%s:fan[%d] rear_failed=%d", __func__, fan, fan_info->rear_failed);
				}
				if(fantray->failed > 0)
					fan_failure++;
				else if(fan < TOTAL_FANS && fantray->present == 0)
					fan_failure++;

				write_fan_led(fan, FAN_LED_RED);
			}
		}
		syslog(LOG_DEBUG, "[zmzhan]%s: fan_failure=%d, sub_failed=%d", __func__, fan_failure, sub_failed);
		if(sub_failed >= 2) {
			fan_failure += sub_failed / 2;
		} else if(sub_failed == 1 && one_failed == 1) {
			if(direction == FAN_DIR_B2F) {
				policy = &b2f_one_fail_policy;
			} else {
				policy = &f2b_one_fail_policy;
			}
			syslog(LOG_DEBUG, "[zmzhan]%s: Change the policy: policy=%p(fail: b2f:%p, f2b:%p)", __func__, policy, &b2f_one_fail_policy, &f2b_one_fail_policy);
		} else {
			if(one_failed == 0 && (policy == &b2f_one_fail_policy || policy == &f2b_one_fail_policy)) {
				if(direction == FAN_DIR_B2F) {
					policy = &b2f_policy;
				} else {
					policy = &f2b_policy;
				}
				syslog(LOG_DEBUG, "[zmzhan]%s: Recovery policy: policy=%p(b2f:%p, f2b:%p)", __func__, policy, &b2f_policy, &f2b_policy);
			}
		}
		if (fan_failure > 0) {
			if (prev_fans_bad != fan_failure) {
				syslog(LOG_CRIT, "%d fans failed", fan_failure);
			}
			fan_speed = FAN_MAX;
			for (fan = 0; fan < TOTAL_FANS; fan++) {
				write_fan_speed(fan, fan_speed);
			}
			write_psu_fan_speed(fan, fan_speed);
			old_speed = fan_speed;
		} else if(prev_fans_bad != 0 && fan_failure == 0){
			old_speed = 0;
		} else {			
			old_speed = fan_speed;
		}
		/* Suppress multiple warnings for similar number of fan failures. */
		prev_fans_bad = fan_failure;

		/* if everything is fine, restart the watchdog countdown. If this process
		 * is terminated, the persistent watchdog setting will cause the system
		 * to reboot after the watchdog timeout. */
		//kick_watchdog(); //zmzhan remove temp
		usleep(11000);
	}

	return 0;
}

