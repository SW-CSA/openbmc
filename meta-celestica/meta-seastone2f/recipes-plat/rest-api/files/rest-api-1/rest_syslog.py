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

from subprocess import Popen, PIPE
import json
import re
import subprocess
import bmc_command

def syslog_action(data):
    result = []
    length = len(data)
    addr = data['addr']
    port = '514'

    if length >= 2:
        port = data['port']

    cmd = 'source /usr/local/bin/openbmc-utils.sh; upgrade_syslog_server ' + addr + ' ' + port
    proc = subprocess.Popen([cmd],
                            shell=True, stdout=PIPE, stderr=PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode().split('\n')
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    if len(data) >= 2:
        if data[0].strip() == 'stopping rsyslogd ... done' and \
           data[1].strip() == 'starting rsyslogd ... done':
            result = {"result": "success"}
        else:
            result = {"result": "fail"}
    else:
        result = {"result": "fail"}

    return result

