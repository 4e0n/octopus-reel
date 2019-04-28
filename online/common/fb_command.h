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

/* The constants and structures related with frontend-backend communication. */

#ifndef FB_COMMAND_H
#define FB_COMMAND_H

/* Commands sent to/from ACQ/STIM backend from/to Frontend daemon. */

/* Global */
#define	ACT_START		(0x0001)
#define	ACT_STOP		(0x0002)
#define	ACT_PAUSE		(0x0003) /* Useful in psychophysical stim */
#define CMD_F2B			(0x0004)
#define CMD_B2F			(0x0005)
#define MSG_ALERT		(0x0006) /* a.k.a. Problem in backend! */
#define	B2F_DATA_SYN		(0x1001) /* Xfer handshake req. from backend */
#define	F2B_DATA_ACK		(0x1002) /* Xfer handshake ack. from frntend */ 
#define	F2B_RESET_SYN		(0x1003) /* Invoke backend rst from frontend */
#define	B2F_RESET_ACK		(0x1004) /* Frontend approval of reset */
#define F2B_GET_BUF_SIZE	(0x1005) /* Buffer size alloc'd in backend */
#define B2F_PUT_BUF_SIZE	(0x1006)

/* ACQ Specific */
#define F2B_GET_TOTAL_COUNT	(0x2001) /* N+2 */
#define B2F_PUT_TOTAL_COUNT	(0x2002)
#define F2B_TRIGTEST		(0x2011)
#define ALERT_DATA_LOSS		(0x3001) /* Buffer underrun ->
                                            Data is not being fetched fast
				            enough by the frontend.. */
/* STIM Specific */
#define	STIM_SET_PARADIGM	(0x4001)
#define	STIM_LOAD_PATTERN	(0x4010)
#define STIM_PATT_XFER_SYN	(0x5001) /* Handshake for Xfering pattern */
#define STIM_PATT_XFER_ACK	(0x5002) /* to backend over SHM. */
#define	STIM_TRIG_START		(0x6004)
#define	STIM_TRIG_STOP		(0x6005)
#define	STIM_SYNTH_EVENT	(0x6001) /* Diagnostic synthetic event
                                            coming from operator computer. */
#define STIM_SET_IIDITD_T1	(0x6101) /* Default is  1ms. */
#define STIM_SET_IIDITD_T2	(0x6102) /* Default is 10ms. */
#define STIM_SET_IIDITD_T3	(0x6103) /* Default is 50ms. */

/* Command Message for the frontend <-> backend communication. */
typedef struct _fb_command { unsigned short id; int iparam[4]; } fb_command;

#endif
