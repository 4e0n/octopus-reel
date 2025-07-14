/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#ifndef TCPCOMMAND_H
#define TCPCOMMAND_H

const int CMD_ACQINFO=0x0001;
const QString CMD_ACQINFO_S="ACQINFO";
const int CMD_TRIGGER=0x0002;
const QString CMD_TRIGGER_S="TRIGGER";
const int CMD_AMPSYNC=0x0003;
const QString CMD_AMPSYNC_S="AMPSYNC";
const int CMD_STATUS=0x0004;
const QString CMD_STATUS_S="STATUS";

const int CMD_DISCONNECT=0x000f;
const QString CMD_DISCONNECT_S="DISCONNECT";

const int CMD_GETCONF=0x1001;
const QString CMD_GETCONF_S="GETCONF";
const int CMD_GETCHAN=0x1002;
const QString CMD_GETCHAN_S="GETCHAN";

const int CMD_REBOOT=0xfffe;
const QString CMD_REBOOT_S="REBOOT";
const int CMD_SHUTDOWN=0xffff;
const QString CMD_SHUTDOWN_S="SHUTDOWN";

#endif
