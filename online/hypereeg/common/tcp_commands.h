/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

#pragma once

// node-acq
const QString CMD_ACQ_ACQINFO="ACQINFO";
const QString CMD_ACQ_AMPSYNC="AMPSYNC";
const QString CMD_ACQ_STATUS="STATUS";
const QString CMD_ACQ_DISCONNECT="DISCONNECT";
const QString CMD_ACQ_GETCONF="GETCONF";
const QString CMD_ACQ_GETCHAN="GETCHAN";
const QString CMD_ACQ_COMPCHAN="COMPCHAN";
const QString CMD_ACQ_S_TRIG_1000="STRIG1000";
const QString CMD_ACQ_S_TRIG_1="STRIG1";
const QString CMD_ACQ_S_TRIG_2="STRIG2";
const QString CMD_ACQ_DUMPON="DUMP=1";
const QString CMD_ACQ_DUMPOFF="DUMP=0";
const QString CMD_ACQ_QUIT="QUIT";
const QString CMD_ACQ_REBOOT="REBOOT";
const QString CMD_ACQ_SHUTDOWN="SHUTDOWN";
//const QString CMD_ACQ_TRIGGER="TRIGGER";
//const QString CMD_ACQ_STOP_EEGSTRM="STOPEEGSTRM";

// node-stor
const QString CMD_STOR_STATUS="STATUS";
const QString CMD_STOR_REC_ON="RECSTART";
const QString CMD_STOR_REC_OFF="RECSTOP";

// node-time
const QString CMD_TIME_ACQINFO="ACQINFO";
const QString CMD_TIME_TRIGGER="TRIGGER";
const QString CMD_TIME_AMPSYNC="AMPSYNC";
const QString CMD_TIME_STATUS="STATUS";
const QString CMD_TIME_STOP_EEGSTRM="STOPEEGSTRM";
const QString CMD_TIME_DISCONNECT="DISCONNECT";
const QString CMD_TIME_GETCONF="GETCONF";
const QString CMD_TIME_GETCHAN="GETCHAN";
