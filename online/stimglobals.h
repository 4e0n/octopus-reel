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

/* The constants used in common between the stimulation backend and daemon.
   Since this file is included both from kernel space backend C, and the
   frontend C++ routines, common convention is preferred for the definitions.*/

#ifndef STIM_GLOBALS_H
#define STIM_GLOBALS_H

//#define DACZERO16	/* Default is 16-bit DAC samplespace */

/* FIFOs for frontend<->backend communication of stim. */
#ifdef OCTOPUS_RTAI
#define STIM_F2BFIFO	(2)
#define STIM_B2FFIFO	(3)
#endif

/* DAQCard dependent settings */
#ifdef DACZERO16
#define DACZERO		(32768)		/* Zero-level for 16-bit DAC. */
#define AMP_L20		(16*204)	/*  -20dB of Reference */
#define AMP_0DB		(16*647)	/* Reference Audio Level */
#define AMP_H20		(16*2045)	/*  +20dB of Reference */
#define AMP_OPPCHN	(16*170)	/* Calibrated Etymotics 100dB */
#else
#define DACZERO		(2048)		/* Zero-level for 12-bit DAC. */
#define AMP_L20		(16*204)>>4	/*  -20dB of Reference */
#define AMP_0DB		(16*647)>>4	/* Reference Audio Level */
#define AMP_H20		(16*2045)>>4	/*  +20dB of Reference */
#define AMP_OPPCHN	(16*170)>>4	/* Calibrated Etymotics 100dB */
#endif

#define PHASE_LL	(50)		/* Left  is leading */
#define PHASE_RL	(-50)		/* Right is leading */

#define OCTOPUS_COMEDI_DIO_SUBDEV	(2)

/* Highest 128M of 1G RAM is dedicated to stimulus pattern
   "memmap=128M!896M" should exist in kernel cmdline */
#define HIMEMSTART			((1024-128)*0x100000)

/* Shared Memory between backend and frontend. */
#define XFERBUFSIZE			(16)

/* Size of stimulus presentation pattern buffer size. */
#define PATTBUFSIZE			(64) /* (8) */

#define AUDIO_RATE			(50000)	/* Audio synthesis rate */
#define TRIG_RATE			(9600)	/* Serial trigger rate (bitbang) */

/* Commands for frontend<->backend communication of stim. */
#define	STIM_SET_PARADIGM		(0x0001)

/* --------------------------------------------------- */
#define	STIM_LOAD_PATTERN		(0x0010)

/* --------------------------------------------------- */
#define	STIM_START			(0x1001)
#define	STIM_STOP			(0x1002)
#define	STIM_PAUSE			(0x1003) /* Useful particularly in pausing
						    automatically in between blocks or
                                	            sophisticated psychophysics experiments. */
#define	STIM_RESUME			(0x1004)

#define	TRIG_START			(0x1101)
#define	TRIG_STOP			(0x1102)

/* --------------------------------------------------- */
#define STIM_XFER_SYN			(0x2001) /* Handshake for Xfering data */
#define STIM_XFER_ACK			(0x2002) /* to backend over SHM. */

#define STIM_SET_PARAM_P1		(0x2101) /* Default is  1ms. */
#define STIM_SET_PARAM_P2		(0x2102) /* Default is 10ms. */
#define STIM_SET_PARAM_P3		(0x2103) /* Default is 50ms. */
#define STIM_SET_PARAM_P4		(0x2104) /* Default is 50ms. */
#define STIM_SET_PARAM_P5		(0x2105) /* Default is 50ms. */

#define STIM_LIGHTS_ON			(0x2201)
#define STIM_LIGHTS_DIMM		(0x2202)
#define STIM_LIGHTS_OFF			(0x2203)
/* --------------------------------------------------- */
#define	STIM_SYNTH_EVENT		(0x3001) /* Diagnostic synthetic event
                                	            coming from client application. */

/* --------------------------------------------------- */
#define	STIM_RST_SYN			(0xF001) /* Reset kernel-space backend. */
#define	STIM_RST_ACK			(0xF002)

#endif
