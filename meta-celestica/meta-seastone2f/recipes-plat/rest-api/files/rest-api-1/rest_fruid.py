#!/usr/bin/env python3
#
# Copyright 2016-present Facebook. All Rights Reserved.
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

def get_fruid_status():
    result = []
    fresult = []
    proc = subprocess.Popen(['/usr/local/bin/fru-util status'],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    for adata in data.split('\n\n'):
        sresult = {}
        for edata in adata.split('\n'):
            tdata = edata.split(':')
            if(len(tdata) < 2):
                continue
            sresult[tdata[0].strip()] = tdata[1]
        if sresult != {}:
            result.append(sresult)

    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }

    return fresult

def get_fruid_psu():
    result = []
    fresult = []
    proc = subprocess.Popen(['/usr/local/bin/fru-util psu -a'],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    for adata in data.split('\n\n'):
        sresult = {}
        for edata in adata.split('\n'):
            tdata = edata.split(':')
            if(len(tdata) < 2):
                continue
            sresult[tdata[0].strip()] = tdata[1].strip()
        if sresult != {}:
            result.append(sresult)

    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }

    return fresult

def get_fruid_fan():
    fresult = []
    result = []
    fan_num = 4
    cmd = 'source /usr/local/bin/openbmc-utils.sh; board_type'
    proc = subprocess.Popen([cmd],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error
    if not data.find('Fishbone'):
        fan_num = 4
    elif not data.find('Phalanx'):
        fan_num = 5
    
    for i in range(fan_num):
        cmd = '/usr/local/bin/fruid-util fan' + str(i + 1) 
        proc = subprocess.Popen([cmd],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
        try:
            data, err = bmc_command.timed_communicate(proc)
            data = data.decode()
        except bmc_command.TimeoutError as ex:
            data = ex.output
            err = ex.error

        for adata in data.split('\n\n'):
            sresult = []
            for edata in adata.split('\n'):
                if len(edata) == 0:
                    continue
                sresult.append(edata)
            result.append(sresult)

    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }

    return fresult

def get_fruid_sys():
    result = []
    fresult = []
    proc = subprocess.Popen(['/usr/local/bin/fru-util sys'],
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    for edata in data.split('\n'):
        if len(edata) == 0:
            continue
        result.append(edata)

    fresult = {
                "Information": result,
                "Actions": [],
                "Resources": [],
              }

    return fresult
