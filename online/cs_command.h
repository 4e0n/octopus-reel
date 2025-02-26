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

/* Commands and other constants related with client-server communication. */

#ifndef CS_COMMAND_H
#define CS_COMMAND_H

/* -------------------------------------------------- */
#define CS_STIM_SET_PARADIGM		(0x0001)

/* -------------------------------------------------- */
#define CS_STIM_START			(0x1001)
#define CS_STIM_STOP			(0x1002)
#define CS_STIM_PAUSE			(0x1003)
#define CS_STIM_RESUME			(0x1004)

#define CS_TRIG_START			(0x1101)
#define CS_TRIG_STOP			(0x1102)

/* -------------------------------------------------- */
#define CS_STIM_LOAD_PATTERN_SYN	(0x2001)
#define CS_STIM_LOAD_PATTERN_ACK	(0x2002)

#define CS_STIM_SET_PARAM_P1		(0x2101)
#define CS_STIM_SET_PARAM_P2		(0x2102)
#define CS_STIM_SET_PARAM_P3		(0x2103)
#define CS_STIM_SET_PARAM_P4		(0x2104)
#define CS_STIM_SET_PARAM_P5		(0x2105)

#define CS_STIM_LIGHTS_ON		(0x2201)
#define CS_STIM_LIGHTS_DIMM		(0x2202)
#define CS_STIM_LIGHTS_OFF		(0x2203)
/* -------------------------------------------------- */
#define CS_STIM_SYNTHETIC_EVENT		(0x3001)

/* -------------------------------------------------- */
#define CS_ACQ_INFO			(0x0001)
#define CS_ACQ_INFO_RESULT		(0x0002)
#define CS_ACQ_SYNC_TRIG		(0x0010)
#define CS_ACQ_SYNC_TRIG_ACK		(0x0011)
#define CS_ACQ_TRIGTEST			(0x1001)

/* -------------------------------------------------- */
#define CS_ACQ_SETMODE			(0x5001)
/* -------------------------------------------------- */
#define CS_REBOOT			(0xFFFE)
#define CS_SHUTDOWN			(0xFFFF)

typedef struct _cs_command { /* Client-server communication structure. */
 unsigned short cmd; int iparam[20]; float fparam[20];
} cs_command;

#endif
