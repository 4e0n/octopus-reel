/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

/* The constants used in common between the stimulation backend and daemon.
   Since this file is included both from kernel space backend C, and the
   frontend C++ routines, common convention is preferred for the definitions.*/

#ifndef OCTOPUS_STIM_H
#define OCTOPUS_STIM_H

/* FIFOs for frontend<->backend communication of stim. */
#define FBFIFO			(0)
#define BFFIFO			(1)

#define HIMEMSTART		(128*0x100000) /* Upper 128M is dedicated to
                                                  stimulus pattern. */
#define SHMBUFSIZE		(128)	/* Shared Memory between backend and
                                           frontend. */
#define DA_SYNTH_RATE		(50000)	/* Audio synthesis rate (main loop). */

/* Commands for frontend<->backend communication of stim. */
#define	STIM_SET_PARADIGM	(0x0001)

#define	STIM_LOAD_PATTERN	(0x0010)

#define STIM_PATT_XFER_SYN	(0x1001) /* Handshake for Xfering pattern */
#define STIM_PATT_XFER_ACK	(0x1002) /* to backend over SHM. */

#define	STIM_START		(0x2001)
#define	STIM_STOP		(0x2002)
#define	STIM_PAUSE		(0x2003) /* Useful in psychophysics
                                            experiments. */
#define	TRIG_START		(0x2004)
#define	TRIG_STOP		(0x2005)

#define	STIM_SYNTH_EVENT	(0x3001) /* Diagnostic synthetic event
                                            coming from operator computer. */

#define STIM_SET_IIDITD_T1	(0x1101) /* Default is  1ms. */
#define STIM_SET_IIDITD_T2	(0x1102) /* Default is 10ms. */
#define STIM_SET_IIDITD_T3	(0x1103) /* Default is 50ms. */

#define	STIM_RST_SYN		(0xF001) /* Reset kernel-space backend. */
#define	STIM_RST_ACK		(0xF002)

#endif
