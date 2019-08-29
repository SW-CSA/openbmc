/*
 * Copyright 2016-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openbmc/pal.h>
#include <string.h>
#include <dirent.h>

#define CMD_SET_FAN_STR "--set"
#define CMD_GET_FAN_STR "--get"
#define ALL_FAN_NUM     0xFF
#define SENSOR_FAIL_RECORD_DIR "/tmp/sensorfail_record"
#define FAN_FAIL_RECORD_DIR "/tmp/fanfail_record"
#define SENSOR_FAIL_FILE "/tmp/cache_store/sensor_fail_boost"
#define FAN_FAIL_FILE "/tmp/cache_store/fan_fail_boost"
#define FAN_MODE_FILE "/tmp/cache_store/fan_mode"

enum {
  CMD_SET_FAN = 0,
  CMD_GET_FAN,
};

enum {
  NORMAL = 0,
  TRANSIT = 1,
  BOOST = 2,
};

static void
print_usage(void) {
  char pwm_list[16] = "";
  char tach_list[16] = "";

  strncpy(pwm_list, pal_get_pwn_list(),16);
  strncpy(tach_list, pal_get_tach_list(),16);
  printf("Usage: fan-util --set <[0..100] %%> < Fan# [%s] >\n", pwm_list);
  printf("       fan-util --get < Fan# [%s] >\n", tach_list);
}

static int
parse_pwm(char *str, uint8_t *pwm) {
  char *endptr = NULL;
  int val;

  val = strtol(str, &endptr, 10);
  if ((*endptr != '\0' && *endptr != '%') || val > 100 || val < 0)
    return -1;
  *pwm = (uint8_t)val;
  return 0;
}

static int
parse_fan(char *str, uint8_t fan_cnt, uint8_t *fan) {
  int val;
  char *endptr = NULL;

  val = strtol(str, &endptr, 10);
  if (*endptr != '\0' || val >= fan_cnt || val < 0)
    return -1;
  *fan = (uint8_t)val;
  return 0;
}

static void
sensor_fail_check(bool status) {
  DIR *dir;
  struct dirent *ptr;
  int cnt = 0;

  if (status) { 
    printf("Sensor Fail: Not support in manual mode(No fscd running)\n"); 
    return;
  }

  if (access(SENSOR_FAIL_FILE, F_OK) == 0) {
    dir = opendir(SENSOR_FAIL_RECORD_DIR);
    if (dir != NULL) {
      printf("Sensor Fail: ");
      while((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)  
          continue;
        cnt++;
        if (cnt == 1) {
          printf("\n%s\n", ptr->d_name);
          continue;
        }
        printf("%s\n", ptr->d_name); 
      }
      closedir(dir);
      if (cnt == 0)
        printf("None\n");
    } else {
      printf("Sensor Fail: None\n");
    }    
  } else {
    printf("Sensor Fail: None\n");
  } 
  return;
}

static void
fan_fail_check(bool status) {
  DIR *dir;
  struct dirent *ptr;
  int cnt = 0;

  if (status) {
    printf("Fan Fail: Not support in manual mode(No fscd running)\n");
    return;
  }

  if (access(FAN_FAIL_FILE, F_OK) == 0) {
    dir = opendir(FAN_FAIL_RECORD_DIR);
    if (dir != NULL) {
      printf("Fan Fail: ");
      while((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
          continue;
        cnt++;
        if (cnt == 1) {
          printf("\n%s\n", ptr->d_name);
          continue;
        }
        printf("%s\n", ptr->d_name);
      }
      closedir(dir);
      if (cnt == 0)
        printf("None\n");
    } else {
      printf("Fan Fail: None\n");
    }
  } else {
    printf("Fan Fail: None\n");
  }
  return;
}

static bool
fan_mode_check(void) {
  FILE* fp;
  char cmd[128];
  char buf[32];
  int res; 
  int fd; 
  uint8_t mode; 
  int cnt;

  sprintf(cmd, "ps | grep /usr/bin/fscd.py | wc -l"); 
  if((fp = popen(cmd, "r")) == NULL) {
    printf("Fan Mode: Unknown\n");
    return false;
  }

  if(fgets(buf, sizeof(buf), fp) != NULL) {
    res = atoi(buf);
    if(res <= 2) {  
      printf("Fan Mode: Manual(No fscd running)\n");
      pclose(fp);
      return true;
    }
  }
  pclose(fp);

  fd = open(FAN_MODE_FILE, O_RDONLY);
  if (fd < 0) {
    printf("Fan Mode: Unknown\n");
    return false;
  }

  cnt = read(fd, &mode, sizeof(uint8_t));
  
  if (cnt <= 0) {
    printf("Fan Mode: Unknown\n");
    close(fd);
    return false;
  }

  mode = mode - '0';
  switch(mode) {
    case NORMAL:
      printf("Fan Mode: Normal\n");
      break;   
    case TRANSIT:
      printf("Fan Mode: Transitional\n");
      break;
    case BOOST:
      printf("Fan Mode: Boost\n");
      break;
    default:
      printf("Fan Mode: Unknown\n");
      break;
  }

  close(fd);
  return false;
}

int
main(int argc, char **argv) {

  uint8_t cmd;
  uint8_t pwm;
  uint8_t fan = ALL_FAN_NUM;
  int i;
  int ret;
  int rpm = 0;
  char fan_name[32];
  bool manu_flag = false;
  int pwm_cnt = 0;
  int tach_cnt = 0;

  if (argc < 2 || argc > 4) {
    print_usage();
    return -1;
  }

  pwm_cnt = pal_get_pwm_cnt();
  tach_cnt = pal_get_tach_cnt();

  if ((!strcmp(argv[1], CMD_SET_FAN_STR)) && (argc == 3 || argc == 4)) {

    /* fan-util --set <pwm> <fan#> */
    cmd = CMD_SET_FAN;
    if (parse_pwm(argv[2], &pwm)) {
      print_usage();
      return -1;
    }

    // Get Fan Number, if mentioned
    if (argc == 4) {
      if (parse_fan(argv[3], pwm_cnt, &fan)) {
        print_usage();
        return -1;
      }
    }

  } else if (!strcmp(argv[1], CMD_GET_FAN_STR) && (argc == 2 || argc == 3)) {

    /* fan-util --get <fan#> */
    cmd = CMD_GET_FAN;

    // Get Fan Number, if mentioned
    if (argc == 3) {
      if (parse_fan(argv[2], tach_cnt, &fan)) {
        print_usage();
        return -1;
      }
    }

  } else {
    print_usage();
    return -1;
  }

  if (cmd == CMD_SET_FAN) {
    // Set the Fan Speed.
    // If the fan num is not mentioned, set all fans to given pwm.
    for (i = 0; i < pwm_cnt; i++) {

      // Check if the fan number is mentioned.
      if (fan != ALL_FAN_NUM && fan != i)
          continue;

      ret = pal_set_fan_speed(i, pwm);
      if (ret == PAL_ENOTREADY) {
        printf("Blocked because host power is off\n");
      } else if (!ret) {
        printf("Setting Zone %d speed to %d%%\n", i, pwm);
      } else
        printf("Error while setting fan speed for Zone %d\n", i);
    }

  } else if (cmd == CMD_GET_FAN) {

    for (i = 0; i < tach_cnt; i++) {

      // Check if the fan number is mentioned.
      if (fan != ALL_FAN_NUM && fan != i)
          continue;

      // Get the Fan Speed
      ret = pal_get_fan_speed(i, &rpm);
      pwm = 0;
      if (!ret) {
        memset(fan_name, 0, 32);
        pal_get_fan_name(i, fan_name);
        if ((pal_get_pwm_value(i, &pwm)) == 0)
          printf("%s Speed: %d RPM (%d%%)\n", fan_name, rpm, pwm);
        else {
          printf("Error while getting fan PWM for Fan %d\n", i);
          printf("%s Speed: %d RPM\n", fan_name, rpm);
        }
      } else {
        printf("Error while getting fan speed for Fan %d\n", i);
      }
    }
  }

  if ((cmd == CMD_GET_FAN) && (argc == 2)) {
    manu_flag = fan_mode_check();
    sensor_fail_check(manu_flag);
    fan_fail_check(manu_flag);
    pal_specific_plat_fan_check(manu_flag);
  }

  return 0;
}
