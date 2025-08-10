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

const int CMD_ACQINFO_B=0x0001;
const QString CMD_ACQINFO="ACQINFO";
const int CMD_TRIGGER_B=0x0002;
const QString CMD_TRIGGER="TRIGGER";
const int CMD_AMPSYNC_B=0x0003;
const QString CMD_AMPSYNC="AMPSYNC";
const int CMD_STATUS_B=0x0004;
const QString CMD_STATUS="STATUS";
const int CMD_STOP_EEGSTRM_B=0x0005;
const QString CMD_STOP_EEGSTRM="STOPEEGSTRM";

const int CMD_DISCONNECT_B=0x000f;
const QString CMD_DISCONNECT="DISCONNECT";

const int CMD_GETCONF_B=0x1001;
const QString CMD_GETCONF="GETCONF";
const int CMD_GETCHAN_B=0x1002;
const QString CMD_GETCHAN="GETCHAN";

const int CMD_REBOOT_B=0xfffe;
const QString CMD_REBOOT="REBOOT";
const int CMD_SHUTDOWN_B=0xffff;
const QString CMD_SHUTDOWN="SHUTDOWN";

#endif
