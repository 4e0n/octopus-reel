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

#ifndef CMD_ACQD_H
#define CMD_ACQD_H

const QString CMD_ACQD_ACQINFO="ACQINFO";
const QString CMD_ACQD_TRIGGER="TRIGGER";
const QString CMD_ACQD_AMPSYNC="AMPSYNC";
const QString CMD_ACQD_STATUS="STATUS";
const QString CMD_ACQD_STOP_EEGSTRM="STOPEEGSTRM";

const QString CMD_ACQD_DISCONNECT="DISCONNECT";

const QString CMD_ACQD_GETCONF="GETCONF";
const QString CMD_ACQD_GETCHAN="GETCHAN";

const QString CMD_ACQD_DUMPRAWON="RAWDUMP=1";
const QString CMD_ACQD_DUMPRAWOFF="RAWDUMP=0";

const QString CMD_ACQD_REBOOT="REBOOT";
const QString CMD_ACQD_SHUTDOWN="SHUTDOWN";

#endif
