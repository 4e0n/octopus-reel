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

// common
const QString CMD_DISCONNECT="DISCONNECT"; // DISCONNECT and QUIT are essentially identical
const QString CMD_QUIT="QUIT";
const QString CMD_REBOOT="REBOOT";
const QString CMD_SHUTDOWN="SHUTDOWN";
const QString CMD_STATUS="STATUS";

const QString CMD_GUI_SHOW="GUISHOW";
const QString CMD_GUI_HIDE="GUIHIDE";
const QString CMD_GUI_RAISE="GUIRAISE";
const QString CMD_GUI_REFRESH="GUIREFRESH";
const QString CMD_GUI_START="GUISTART";
const QString CMD_GUI_STOP="GUISTOP";
const QString CMD_GUI_SETREFRESH="GUISETREFRESH"; // rrate
const QString CMD_GUI_SETGEOMETRY="GUISETGEOMETRY"; // x,y,w,h
const QString CMD_GUI_RELOADCONF="GUIRELOADCONF";
const QString CMD_GUI_PALETTE="GUIPALETTE"; // MEAN|MEDIAN
const QString CMD_GUI_SCREENSHOT="GUISCREENSHOT";

// node-acq
const QString CMD_ACQ_ACQINFO="ACQINFO";
const QString CMD_ACQ_AMPSYNC="AMPSYNC";
const QString CMD_ACQ_GETCONF="GETCONF";
const QString CMD_ACQ_GETCHAN="GETCHAN";
const QString CMD_ACQ_COMPCHAN="COMPCHAN";
const QString CMD_ACQ_S_TRIG_1="STRIG1";
const QString CMD_ACQ_S_TRIG_2="STRIG2";
const QString CMD_ACQ_DUMPON="DUMP=1";
const QString CMD_ACQ_DUMPOFF="DUMP=0";
const QString CMD_ACQ_TRIGGER="TRIG";
const QString CMD_ACQ_OPEVT="OPEVT";
const QString CMD_ACQ_SUBEVT="SUBEVT";

// node-stor
const QString CMD_STOR_REC_ON="RECSTART";
const QString CMD_STOR_REC_OFF="RECSTOP";

// node-comp-pp
const QString CMD_COMPPP_GETCMLEVELS="GETCMLEVELS";

// node-wavplay
const QString CMD_WAVPLAY_PLAY="PLAY";
const QString CMD_WAVPLAY_STOP="STOP";
const QString CMD_WAVPLAY_SETSINK="SETSINK";
const QString CMD_WAVPLAY_LIST="LIST";
const QString CMD_WAVPLAY_PLAYSTAT="PLAYSTAT";
