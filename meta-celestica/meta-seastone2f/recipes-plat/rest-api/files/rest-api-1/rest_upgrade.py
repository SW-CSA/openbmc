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
import re
import os
import pexpect
import bmc_command
import json
import subprocess
import sys

def get_remote_file(file_path, password, md5sum = 0):
    cmd = 'scp ' + file_path + ' /tmp/'
    scp = pexpect.spawn(cmd)
    ret = scp.expect([pexpect.TIMEOUT, 'yes', '(password)', 'known_hosts', pexpect.EOF], timeout=60)
    if ret == 0 or ret == 4:
        return {"result": "failed", "Description": "connect {} timeout or EOF".format(file_path), False:True}
    if ret == 1:
        scp.sendline("yes")
        ret = scp.expect([pexpect.TIMEOUT, '(password)', pexpect.EOF], timeout=60)
        if ret == 1:
            scp.sendline(password)
        else:
            return {"result": "failed", "Description": "passowrd confirm fail", False:True}
        ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
        if ret != 1:
            return {"result": "failed", "Description": "transfer file fail", False:True}
    elif ret == 2:
        scp.sendline(password)
        ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
        if ret != 1:
            return {"result": "failed", "Description": "transfer file fail", False:True}
    elif ret == 3:
        os.popen("rm /home/root/.ssh/known_hosts")
        scp = pexpect.spawn(cmd)
        ret = scp.expect([pexpect.TIMEOUT, 'yes', '(password)', pexpect.EOF], timeout=60)
        if ret == 1:
            scp.sendline("yes")
            ret = scp.expect([pexpect.TIMEOUT, '(password)', pexpect.EOF], timeout=60)
            if ret == 1:
                scp.sendline(password)
            else:
                return {"result": "failed", "Description": "passowrd confirm fail", False:True}
            ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
            if ret != 1:
                return {"result": "failed", "Description": "transfer file fail", False:True}
        elif ret == 2:
            scp.sendline(password)
            ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
            if ret != 1:
                return {"result": "failed", "Description": "transfer file fail", False:True}
        else:
            return {"result": "failed", "Description": "connect timeout or EOF", False:True}
    else:
        return {"result": "failed", "Description": "authentication fail", False:True}

    file_name = file_path.split("/")[-1]
    if file_name == '':
        return {"result": "failed", "Description": "file name error", False:True}
    image_path = '/tmp/{}'.format(file_name)
    if md5sum == 0:
        return {"path":image_path}
    cmd = 'md5sum {}'.format(image_path)
    (data, _) = Popen(cmd, shell=True, stdout=PIPE).communicate()
    data = data.decode()
    if file_path.split(" ")[0] == md5sum:
        return {"path":image_path}
    else:
        return {"result": "failed", "Description": "file md5sum not match", False:True}

# BMC upgrade
def bmc_upgrade(data):
    return {"result":"failed", "Description": "Not support yet", False:True}

# BIOS upgrade
def bios_upgrade(flash, image_path, reboot):
    result = {}
    cmd = 'source /usr/local/bin/openbmc-utils.sh'
    flash_type = ['W25Q128.V', 'MX25L12805D']
    try:
        for flashtype in flash_type:
            (data, _) = Popen('{0}; (bios_upgrade {1} {2} | grep Found | grep {2}; echo $?) | tail -n 1'.format(cmd, flash, flashtype),
                        shell=True, stdout=PIPE).communicate(timeout=100)
            data = data.decode()
            if data.strip() == '0':
                (data, _) = Popen('{}; bios_upgrade {} {} w {}'.format(cmd, flash, flashtype, image_path),
                            shell=True, stdout=PIPE).communicate(timeout=600)
                data = data.decode()
                if data.find('Verifying') != -1 and data.find('VERIFIED') != -1:
                    if reboot == 'yes':
                        Popen('{}; come_reset {}'.format(cmd, flash),
                            shell=True, stdout=PIPE).communicate(timeout=20)
                    else:
                        pass
                    (data, _) = Popen('echo {}: success, firmware: {}, time: $(date) >> '
                                '/var/log/bios_upgrade.log'.format(flash, image_path.split("/")[-1]),
                                shell=True, stdout=PIPE).communicate()
                    result = {"result":"success"}
                elif data.find('identical') != -1:
                    (data, _) = Popen('echo {}: success, firmware: {}, time: $(date) >> '
                                '/var/log/bios_upgrade.log'.format(flash, image_path.split("/")[-1]),
                                shell=True, stdout=PIPE).communicate()
                    result = {"result":"success", "Description":"Chip content is identical to the requested image"}
                else:
                    (data, _) = Popen('echo {}: failed, firmware: {}, time: $(date) >> '
                                '/var/log/bios_upgrade.log'.format(flash, image_path.split("/")[-1]),
                                shell=True, stdout=PIPE).communicate()
                    result = {"result":"failed", "Description":"BIOS upgrade failed",False:True}
                return result
        return {"result":"failed", "Description":"Can't find BIOS flash chip", False:True}
    except TimeoutExpired as exp:
        (data, _) = Popen('echo {}: failed, firmware: {}, time: $(date) >> '
                    '/var/log/bios_upgrade.log'.format(flash, image_path.split("/")[-1]),
                    shell=True, stdout=PIPE).communicate()
        return {"result":"failed", "Description":"BMC bios_upgrade command timeout", False:True}
    return {"result": "failed", False:True}

# CPLD upgrade
def cpld_upgrade(cpld_type, image_path):
    cmd = 'source /usr/local/bin/openbmc-utils.sh'
    result = {}
    
    loop = {"fan":"loop1", "switch":"loop1", "cpu":"loop1", "base":"loop1", \
            "combo":"loop1", "enable":"loop1","top_lc":"loop2", "bottom_lc":"loop3"}
    operation = {"fan":"none", "switch":"none", "cpu":"power_on", "base":"power_on", \
            "combo":"none", "enable":"power_on","top_lc":"none", "bottom_lc":"none"}
    try:
        (data, _) = Popen('{}; (cpld_upgrade {} {} | grep PASS; echo $?) | tail -n 1'.format(cmd, 
            loop[cpld_type], image_path), shell=True, stdout=PIPE).communicate(timeout=3600)
        data = data.decode()
        if data.strip() == '0':
            (data, _) = Popen('echo {}: success, firmware: {}, time: $(date) >> '
                '/var/log/cpld_upgrade.log'.format(cpld_type, image_path.split("/")[-1]),
                shell=True, stdout=PIPE).communicate()
            result = {"result": "success"}
        else:
            (data, _) = Popen('echo {}: failed, firmware: {}, time: $(date) >> '
                '/var/log/cpld_upgrade.log'.format(cpld_type, image_path.split("/")[-1]),
                shell=True, stdout=PIPE).communicate()
            return { "result":"failed", "Description":"BMC cpld upgrade failed", False:True }

        if operation[cpld_type] == "none":
            pass
        elif operation[cpld_type] == "power_on":
            (data, _) = Popen('{}; sleep 3; /usr/local/bin/wedge_power.sh on'.format(cmd),
                shell=True, stdout=PIPE).communicate(timeout=5)
        elif operation[cpld_type] == "power_cycle":
            # Only Phalanx support power cycle
            (data, _) = Popen('{}; i2cset -f -y 30 0x20 0x9 0x3f'.format(cmd),
                shell=True, stdout=PIPE).communicate()
        else:
            return { "result":"failed", False:True }
    except KeyError as exp:
        result = { "result": "failed", "Description": "wrong type param: {}".format(exp), False:True }
    except TimeoutExpired as exp:
        (data, _) = Popen('echo {}: failed, firmware: {}, time: $(date) >> '
            '/var/log/cpld_upgrade.log'.format(cpld_type, image_path.split("/")[-1]),
            shell=True, stdout=PIPE).communicate()
        result = { "result": "failed", "Description": "BMC cpld_upgrade command timeout", False:True }

    return result

def upgrade_action(data):
    result = []
    try:
        device = data['device']
        path = data['image_path']
        passowrd = data['password']
        if 'md5sum' in data.keys():
            md5sum = data['md5sum']
        else:
            md5sum = 0
        file_path = get_remote_file(path, passowrd, md5sum)
        if not all(file_path):
            file_path.pop(False)
            return file_path
        if device == 'bmc':
            return {"result":"failed, not support yet"}
            flash = data['flash']
            reboot = data['reboot']
            result = bmc_upgrade(flash, file_path["path"])
        elif device == 'bios':
            flash = data['flash']
            reboot = data['reboot']
            result = bios_upgrade(flash, file_path["path"], reboot)
        elif device == 'cpld':
            cpld_type = data['type']
            result = cpld_upgrade(cpld_type, file_path["path"])
    except  KeyError as exp:
        result = {"result": "failed", "Description": "Lost or wrong param: {}".format(exp)}

    if not all(result):
        result.pop(False)
        return result
    else:
        return result

def get_upgrade_log():
    result = {"result":"success"}
    log_path = {
        "CPLD upgrade log":"/var/log/cpld_upgrade.log",
        "BIOS upgrade log":"/var/log/bios_upgrade.log",
        "BMC upgrade log":"/var/log/bmc_upgrade.log",
    }
    for key in log_path.keys():
        log = []
        if os.path.exists(log_path[key]):
            with open(log_path[key], 'rt') as log_file:
                for line in log_file:
                    log.append(line.strip())
                log_file.close()
            result[key] = log
        else:
            result[key] = 'None'
    
    return result
