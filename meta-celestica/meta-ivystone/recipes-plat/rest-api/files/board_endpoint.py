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
from aiohttp import web
import rest_fruid
import rest_bmc
import rest_sensors
import rest_led
import rest_server
import rest_mTerm
import rest_eth
import rest_raw
import rest_temp
import rest_syslog
import rest_watchdog
import rest_userpassword
import rest_upgrade
from rest_utils import dumps_bytestr, get_endpoints

class boardApp_Handler:

    # Handler for sys/fruid resource endpoint
    def helper_rest_fruid(self, request):
        result = {
            "Information": {
                "Description": "System FRU",
            },
            "Actions": [],
            "Resources": get_endpoints('/api/sys/fruid')
        }
        return web.json_response(result, dumps=dumps_bytestr)

    async def rest_fruid_hdl(self, request):
        return self.helper_rest_fruid(request)

    # Handler for sys/fruid/status resource endpoint
    async def rest_fruid_status_hdl(self, request):
        return web.json_response(rest_fruid.get_fruid_status(), dumps=dumps_bytestr)

    # Handler for sys/fruid/psu resource endpoint
    async def rest_fruid_psu_hdl(self, request):
        return web.json_response(rest_fruid.get_fruid_psu(), dumps=dumps_bytestr)

    # Handler for sys/fruid/fan resource endpoint
    async def rest_fruid_fan_hdl(self, request):
        return web.json_response(rest_fruid.get_fruid_fan(), dumps=dumps_bytestr)

    # Handler for sys/fruid/sys resource endpoint
    async def rest_fruid_sys_hdl(self, request):
        return web.json_response(rest_fruid.get_fruid_sys(), dumps=dumps_bytestr)

    # Handler for sys/bmc resource endpoint
    async def rest_bmc_hdl(self, request):
        return web.json_response(rest_bmc.get_bmc(), dumps=dumps_bytestr)

    # Handler for sys/bmc resource endpoint
    async def rest_bmc_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_bmc.bmc_action(data), dumps=dumps_bytestr)

    # Handler for sys/sensors resource endpoint
    async def rest_sensors_hdl(self, request):
        return web.json_response(rest_sensors.get_sensors(), dumps=dumps_bytestr)

    # Handler for sys/led resource endpoint
    async def rest_led_hdl(self, request):
        return web.json_response(rest_led.get_led(), dumps=dumps_bytestr)

    # Handler for led resource endpoint
    async def rest_led_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_led.led_action(data), dumps=dumps_bytestr)

	# Handler for sys/server resource endpoint
    async def rest_server_hdl(self,request):
        return web.json_response(rest_server.get_server(), dumps=dumps_bytestr)

	# Handler for uServer resource endpoint
    async def rest_server_act_hdl(self,request):
        data = await request.json()
        return web.json_response(rest_server.server_action(data), dumps=dumps_bytestr)

    # Handler to get mTerm status
    async def rest_mTerm_hdl(self, request):
        return web.json_response(rest_mTerm.get_mTerm_status(), dumps=dumps_bytestr)

    # Handler for sys/eth resource endpoint
    async def rest_eth_hdl(self, request):
        return web.json_response(rest_eth.get_eth(), dumps=dumps_bytestr)

    # Handler for eth resource endpoint
    async def rest_eth_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_eth.eth_action(data), dumps=dumps_bytestr)

    # Handler for raw resource endpoint
    async def rest_raw_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_raw.raw_action(data), dumps=dumps_bytestr)

    # Handler for sys/temp resource endpoint
    async def rest_temp_hdl(self, request):
        return web.json_response(rest_temp.get_temp(), dumps=dumps_bytestr)

    # Handler for temp resource endpoint
    async def rest_temp_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_temp.temp_action(data), dumps=dumps_bytestr)

    # Handler for syslog resource endpoint
    async def rest_syslog_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_syslog.syslog_action(data), dumps=dumps_bytestr)

    # Handler for watchdog resource endpoint
    async def rest_wdt_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_watchdog.wdt_action(data), dumps=dumps_bytestr)
    
    # Handler for user password resource endpoint
    async def rest_userpassword_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_userpassword.userpassword_action(data), dumps=dumps_bytestr)

    # Handler for getting upgrade firmware log resource endpoint
    async def rest_upgrade_hdl(self, request):
        return web.json_response(rest_upgrade.get_upgrade_log(), dumps=dumps_bytestr)

    # Handler for upgrading firmware resource endpoint
    async def rest_upgrade_act_hdl(self, request):
        data = await request.json()
        return web.json_response(rest_upgrade.upgrade_action(data), dumps=dumps_bytestr)
