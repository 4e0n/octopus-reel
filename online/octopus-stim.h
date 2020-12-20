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

/* The constants used in common between the stimulation backend and daemon.
   Since this file is included both from kernel space backend C, and the
   frontend C++ routines, common convention is preferred for the definitions.*/

#ifndef OCTOPUS_STIM_H
#define OCTOPUS_STIM_H

/* FIFOs for frontend<->backend communication of stim. */
#define FBFIFO			(0)
#define BFFIFO			(1)

#define HIMEMSTART		(9*128*0x100000) /* Highest 128M of 1G RAM
						    is dedicated to
                                                    stimulus pattern. */
#define SHMBUFSIZE		(128)	/* Shared Memory between backend and
                                           frontend. */
#define AUDIO_RATE		(50000)	/* Audio synthesis rate. */
#define TRIG_RATE		(9600)	/* Serial trigger rate. */

/* Commands for frontend<->backend communication of stim. */
#define	STIM_SET_PARADIGM	(0x0001)

/* --------------------------------------------------- */
#define	STIM_LOAD_PATTERN	(0x0010)

/* --------------------------------------------------- */
#define	STIM_START		(0x1001)
#define	STIM_STOP		(0x1002)
#define	STIM_PAUSE		(0x1003) /* Useful in psychophysics
                                            experiments. */
#define	TRIG_START		(0x1004)
#define	TRIG_STOP		(0x1005)

/* --------------------------------------------------- */
#define STIM_PATT_XFER_SYN	(0x2001) /* Handshake for Xfering pattern */
#define STIM_PATT_XFER_ACK	(0x2002) /* to backend over SHM. */

#define STIM_SET_PARAM_P1	(0x2101) /* Default is  1ms. */
#define STIM_SET_PARAM_P2	(0x2102) /* Default is 10ms. */
#define STIM_SET_PARAM_P3	(0x2103) /* Default is 50ms. */
#define STIM_SET_PARAM_P4	(0x2104) /* Default is 50ms. */
#define STIM_SET_PARAM_P5	(0x2105) /* Default is 50ms. */

/* --------------------------------------------------- */
#define	STIM_SYNTH_EVENT	(0x3001) /* Diagnostic synthetic event
                                            coming from operator computer. */

/* --------------------------------------------------- */
#define	STIM_RST_SYN		(0xF001) /* Reset kernel-space backend. */
#define	STIM_RST_ACK		(0xF002)

#endif
