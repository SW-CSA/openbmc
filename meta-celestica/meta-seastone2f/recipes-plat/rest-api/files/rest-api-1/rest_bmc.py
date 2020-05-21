#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#


from subprocess import Popen, PIPE
import re
import os
import pexpect
import bmc_command
import json
import subprocess
import sys

# Read all contents of file path specified.
def read_file_contents(path):
    try:
        with open(path, 'r') as proc_file:
            content = proc_file.readlines()
    except IOError as e:
        content = None

    return content


# Handler for FRUID resource endpoint
def get_bmc():
    # Get BMC Reset Reason
    (wdt_counter, _) = Popen('devmem 0x1e785010',
                             shell=True, stdout=PIPE).communicate()
    wdt_counter = int(wdt_counter, 0)

    wdt_counter &= 0xff00

    if wdt_counter:
        por_flag = 0
    else:
        por_flag = 1

    if por_flag:
        reset_reason = "Power ON Reset"
    else:
        reset_reason = "User Initiated Reset or WDT Reset"

    # Get BMC's Up Time
    uptime = Popen('uptime', shell=True, stdout=PIPE).stdout.read().decode().strip('\n')

    # Use another method, ala /proc, but keep the old one for backwards
    # compat.
    # See http://man7.org/linux/man-pages/man5/proc.5.html for details
    # on full contents of proc endpoints.
    uptime_seconds = read_file_contents("/proc/uptime")[0].split()[0]

    # Pull load average directory from proc instead of processing it from
    # the contents of uptime command output later.
    load_avg = read_file_contents("/proc/loadavg")[0].split()[0:3]

    # Get Usage information
    (data, _) = Popen('top -b n1',
                      shell=True, stdout=PIPE).communicate()
    data = data.decode()
    adata = data.split('\n')
    mem_usage = adata[0]
    cpu_usage = adata[1]

    # Get disk usage information
    (data, _) = Popen('df | grep mmc',
                       shell=True, stdout=PIPE).communicate()
    data = data.decode()
    adata = data.split()
    if len(adata) >= 5:
        disk_usage = 'EMMC: {}K total, {}K used, {}K available, {} use'.format(adata[1], adata[2], adata[3], adata[4])
    else:
        disk_usage = ""

    # Get OpenBMC version
    version = ""
    (data, _) = Popen('cat /etc/issue',
                      shell=True, stdout=PIPE).communicate()
    data = data.decode()
    ver = re.search(r'v([\w\d._-]*)\s', data)
    if ver:
        version = ver.group(1)

    # Get U-Boot version
    uboot_version = ""
    (data, _) = Popen('strings /dev/mtd0 | grep U-Boot | grep 2016',
                      shell=True, stdout=PIPE).communicate()
    data = data.decode()
    ver = re.search(r'v([\w\d._-]*)\s', data)
    if ver:
        uboot_version = ver.group(1)

    # Get CPLD version
    syscpld_version = ""
    (data, _) = Popen('cat /sys/bus/i2c/devices/0-000d/version | head -n 1',
                       shell=True, stdout=PIPE).communicate()
    data = data.decode()
    if len(data) > 2:
        syscpld_version = '{:0>2}'.format(data[2:].strip('\n'))
    else:
        syscpld_version = 'null'

    fancpld_version = ""
    (data, _) = Popen('cat /sys/bus/i2c/devices/8-000d/version | head -n 1',
                       shell=True, stdout=PIPE).communicate()
    data = data.decode()
    if len(data) > 2:
        fancpld_version = '{:0>2}'.format(data[2:].strip('\n'))
    else:
        fancpld_version = 'null'

    board_type = 'AS58XX-CL'
    (data, _) = Popen('source /usr/local/bin/openbmc-utils.sh; board_type',
                       shell=True, stdout=PIPE).communicate()
    data = data.decode()
    if not data.find('Fishbone48'):
        board_type = 'Fishbone48'
    elif not data.find('Fishbone32'):
        board_type = 'Fishbone32'
    elif not data.find('Phalanx'):
        board_type = 'Phalanx'

    result = {
                "Information": {
                    "Description": "{} BMC".format(board_type),
                    "Reset Reason": reset_reason,
                    # Upper case Uptime is for legacy
                    # API support
                    "Uptime": uptime,
                    # Lower case Uptime is for simpler
                    # more pass-through proxy
                    "uptime": uptime_seconds,
                    "load-1": load_avg[0],
                    "load-5": load_avg[1],
                    "load-15": load_avg[2],
                    "Disk Usage":disk_usage,
                    "Memory Usage": mem_usage,
                    "CPU Usage": cpu_usage,
                    "OpenBMC Version": version,
                    "U-Boot Version": uboot_version,
                    "SysCPLD Version":syscpld_version,
                    "FanCPLD Version":fancpld_version,
                },
                "Actions": [],
                "Resources": [],
             }

    return result

def bmc_action(data):
    ret = 0
    length = len(data)
    reboot = 0
    file_path = ''
    flash = ''
    password = ''
    rb = ''

    if length == 1:
        rb = data['reboot']
        if rb == 'yes' or rb == 'Yes':
            os.popen("reboot")
            return {"result": "reboot BMC"}
    elif length >= 3:
        file_path = data['path']
        flash = data['flash']
        password = data['password']
        if length > 3:
            rb = data['reboot']
            if rb == 'yes' or rb == 'Yes':
                reboot = 1
    else:
        return {"result": "Error: parameters invalid"}

    if file_path == '' or flash == '':
        return {"result": "Error: path or flash is NULL"}

    if password == '':
        return {"result": "Error: passowrd is NULL"}

    cmd = 'scp ' + file_path + ' /tmp/'
    scp = pexpect.spawn(cmd)
    ret = scp.expect([pexpect.TIMEOUT, 'yes', '(password)', 'known_hosts', pexpect.EOF], timeout=60)
    if ret == 0 or ret == 4:
        return {"result": "Error: connect timeout or EOF"}

    if ret == 1:
        scp.sendline("yes")
        ret = scp.expect([pexpect.TIMEOUT, '(password)', pexpect.EOF], timeout=60)
        if ret == 1:
            scp.sendline(password)
        else:
            return {"result": "Error: passowrd confirm fail"}
        ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
        if ret != 1:
            return {"result": "Error: transfer file fail"}
    elif ret == 2:
        scp.sendline(password)
        ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
        if ret != 1:
            return {"result": "Error: transfer file fail"}
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
                return {"result": "Error: passowrd confirm fail"}
            ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
            if ret != 1:
                return {"result": "Error: transfer file fail"}
        elif ret == 2:
            scp.sendline(password)
            ret = scp.expect([pexpect.TIMEOUT, '100%', pexpect.EOF], timeout=60)
            if ret != 1:
                return {"result": "Error: transfer file fail"}
        else:
            return {"result": "Error: connect timeout or EOF"}
    else:
        return {"result": "Error: authentication fail"}

    file_name = file_path.split("/")[-1]
    if file_name == '':
        return {"result": "Error: file name error"}

    if flash == 'master':
        cmd = 'flashcp /tmp/' + file_name + ' /dev/mtd5'
    else:
        cmd = 'flashcp /tmp/' + file_name + ' /dev/mtd11'

    proc = subprocess.Popen([cmd], shell=True,
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        data, err = bmc_command.timed_communicate(proc, 480)
        data = data.decode()
    except bmc_command.TimeoutError as ex:
        data = ex.output
        err = ex.error

    if data != '' and data != '\n':
        (data, _) = Popen('echo {}: failed, firmware: {}, time: $(date) >> '
                    '/var/log/bmc_upgrade.log'.format(flash, file_name),
                    shell=True, stdout=PIPE).communicate()
        return {"result": data}
    else:
        if reboot == 1:
            os.popen("reboot")
            return {"result": "success, and reboot BMC"}
        else:
            if flash == 'master':
                (data, _) = Popen('echo {}: success, firmware: {}, time: $(date) >> '
                            '/var/log/bmc_upgrade.log'.format(flash, file_name),
                            shell=True, stdout=PIPE).communicate()
                return {"result": "success! Do you need to reboot BMC?"}
            else:
                (data, _) = Popen('echo {}: success, firmware: {}, time: $(date) >> '
                            '/var/log/bmc_upgrade.log'.format(flash, file_name),
                            shell=True, stdout=PIPE).communicate()
                return {"result": "success"}

