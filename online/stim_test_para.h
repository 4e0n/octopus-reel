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

/* Some Low-level definitions for STIM DA-converter. */

#define DACZERO16	/* Default is 16-bit DAC samplespace */

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

/* Test and paradigm codes for switching. */
#define TEST_CALIBRATION	(0x0001)
#define TEST_SINECOSINE		(0x0002)
#define TEST_SQUARE		(0x0003)
#define TEST_STEADYSQUARE	(0x0004)
#define PARA_CLICK		(0x1001)
#define PARA_SQUAREBURST	(0x1002)
#define PARA_IIDITD		(0x1003)
#define PARA_IIDITD_PPLP	(0x1004)
#define PARA_ITD_OPPCHN		(0x1005)
#define PARA_ITD_OPPCHN2	(0x1006)
#define PARA_ITD_LINTEST	(0x1007)
#define PARA_ITD_LINTEST2	(0x1008)
#define PARA_ITD_TOMPRES	(0x1009)
#define PARA_ITD_PIP_CTRAIN	(0x100A)
#define PARA_ITD_PIP_RAND	(0x100B)
#define PARA_0021A		(0x100C)
#define PARA_0021B		(0x100D)
