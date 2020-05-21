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
import json
import asyncio
from aiohttp import web
from rest_utils import dumps_bytestr, get_endpoints
from concurrent.futures import ThreadPoolExecutor

def common_force_async(func):
    async def func_wrapper(self, *args, **kwargs):
        # Convert the possibly blocking helper function into async
        loop = asyncio.get_event_loop()
        result = await loop.run_in_executor(self.common_executor, \
                                            func, self, *args, **kwargs)
        return result
    return func_wrapper

class commonApp_Handler:

    # common handler will use its own executor (thread based),
    # we initentionally separated this from the executor of
    # board-specific REST handler, so that any problem in
    # common REST handlers will not interfere with board-specific
    # REST handler, and vice versa
    def __init__(self):
        # Max number of concurrent thread is set to 5,
        # in order to ensure enough concurrency while
        # not overloading CPU too much
        self.common_executor = ThreadPoolExecutor(5)

    # When we call request.json() in asynchronous function, a generator
    # will be returned. Upon calling next(), the generator will either :
    #
    # 1) return the next data as usual,
    #   - OR -
    # 2) throw StopIteration, with its first argument as the data
    #    (this is for indicating that no more data is available)
    #
    # Not sure why aiohttp's request generator is implemented this way, but
    # the following function will handle both of the cases mentioned above.
    def get_data_from_generator(self, data_generator):
        data = None
        try:
            data = next(data_generator)
        except StopIteration as e:
            data = e.args[0]
        return data

    # Handler for root resource endpoint
    def helper_rest_api(self, request):
        result = {
            "Information": {
                "Description": "Fishbone RESTful API Entry",
                "version": "v0.1",
            },
            "Actions": [],
            "Resources": get_endpoints('/api')
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_api(self, request):
        return self.helper_rest_api(request)

    # Handler for root resource endpoint
    def helper_rest_sys(self,request):
        result = {
            "Information": {
                "Description": "AS58XX-CL System",
            },
            "Actions": [],
            "Resources": get_endpoints('/api/sys')
        }
        return web.json_response(result, dumps=dumps_bytestr)

    @common_force_async
    def rest_sys(self,request):
        return self.helper_rest_sys(request)
