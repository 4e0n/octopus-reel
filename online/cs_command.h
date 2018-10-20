/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* Commands and other constants related with client-server communication. */

#ifndef CS_COMMAND_H
#define CS_COMMAND_H

#define CS_STIM_SET_PARADIGM		(0x0001)

#define CS_STIM_LOAD_PATTERN_SYN	(0x1001)
#define CS_STIM_LOAD_PATTERN_ACK	(0x1002)

#define CS_STIM_START			(0x2001)
#define CS_STIM_STOP			(0x2002)
#define CS_STIM_PAUSE			(0x2003)
#define CS_TRIG_START			(0x2004)
#define CS_TRIG_STOP			(0x2005)

#define CS_STIM_SET_IIDITD_T1		(0x1101)
#define CS_STIM_SET_IIDITD_T2		(0x1102)
#define CS_STIM_SET_IIDITD_T3		(0x1103)

#define CS_STIM_SYNTHETIC_EVENT		(0x3001)

#define CS_ACQ_INFO			(0x0001)
#define CS_ACQ_INFO_RESULT		(0x0002)
#define CS_ACQ_TRIGTEST			(0x1001)

#define CS_REBOOT			(0xFFFE)
#define CS_SHUTDOWN			(0xFFFF)

typedef struct _cs_command { /* Client-server communication structure. */
 unsigned short cmd; int iparam[3]; float fparam[3];
} cs_command;

#endif
