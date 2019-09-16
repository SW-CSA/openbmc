#!/usr/bin/env python3
#
# Copyright 2018-present Celestica. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

import json
import re
import subprocess
import bmc_command

def get_led():
    fresult = []
    proc = subprocess.Popen(['source /usr/local/bin/openbmc-utils.sh;'
                            'sys_led'],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    fresult = {
                "Information": data,
                "Actions": [],
                "Resources": [],
              }

    return fresult

def led_action(data):
    result = []
    value = data["action"]
    blink = data["blink"]
    cmd = 'sys_led ' + value + ' ' + blink
    proc = subprocess.Popen(['source /usr/local/bin/openbmc-utils.sh;'
                            + cmd],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    if data == '':
        result = {"result": "success"}
    else:
        result = {"result": "fail"}

    return result
