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


def raw_action(data):
    result = []
    fresult = []
    cmd = data["data"]
    timeout = 10
    if len(data) > 1:
        timeout = data["timeout"]

    proc = subprocess.Popen([cmd],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc, int(timeout))
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error


    if data == '':
        if err != '':
            fresult = {"result": err}
        else:
            fresult = {"result": "success"}
    else:
        for edata in data.split('\n'):
            result.append(edata)
        fresult = {"result": result}

    return fresult
