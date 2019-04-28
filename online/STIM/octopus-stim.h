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
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* The constants used in common between the stimulation backend and daemon.
   Since this file is included both from kernel space backend C, and the
   frontend C++ routines, common convention is preferred for the definitions.*/

#ifndef OCTOPUS_STIM_H
#define OCTOPUS_STIM_H

/* FIFOs for frontend<->backend communication of stim. */
#define STIM_F2BFIFO		(0)
#define STIM_B2FFIFO		(1)


#define HIMEMSTART		(128*0x100000) /* Upper 128M is dedicated to
                                                  stimulus pattern. */
#define SHMBUFSIZE		(128)	/* Shared Memory between backend and
                                           frontend. */
#define DA_SYNTH_RATE		(10000)	/* Audio synthesis rate (main loop). */

#define STIM_PARBASE		(0x378)
#define STIM_PARIRQ		(7)
#define STIM_SERBASE		(0x3f8)
#define STIM_SERIRQ		(4)

#endif
