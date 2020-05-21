#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# restart rsyslog with runsv
#pid=$(ps |grep rsyslogd |grep -v grep | awk -F ' ' '{print $1}')
#if [ -n $pid ]; then
#    kill $pid
#    sleep 1
#fi
#mkdir /etc/sv/rsyslog
#echo "#!/bin/sh" > /etc/sv/rsyslog/run
#echo "exec /usr/sbin/rsyslogd" >> /etc/sv/rsyslog/run
#chmod +x /etc/sv/rsyslog/run
#runsv /etc/sv/rsyslog > /dev/null 2>&1 &
#sv restapi

