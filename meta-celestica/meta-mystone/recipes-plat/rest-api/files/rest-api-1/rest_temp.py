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

def get_temp():
    result = []
    proc = subprocess.Popen(['source /usr/local/bin/openbmc-utils.sh; sys_temp'],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    for edata in data.split('\n\n'):
        adata = edata.split('\n', 1)
        sresult = {}
        if (len(adata) < 2):
            break;
        sresult['name'] = adata[0]
        for sdata in adata[1].split('\n'):
            tdata = sdata.split(':')
            if (len(tdata) < 2):
                continue
            sresult[tdata[0].strip()] = tdata[1]
        result.append(sresult)

    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }

    return fresult

def temp_action(data):
    result = []
    chip = data["chip"]
    option = data["option"]
    value = data["value"]

    if option == "input":
        file_suffix = "input"
    elif option == "max":
        file_suffix = "max"
    elif option == "max hyst":
        file_suffix = "max_hyst"
    elif option == "max_hyst":
        file_suffix = "max_hyst"

    command = '{} {} {} {}'.format('sys_temp', chip, file_suffix, value)
    proc = subprocess.Popen(['source /usr/local/bin/openbmc-utils.sh;'
                            + command],
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
