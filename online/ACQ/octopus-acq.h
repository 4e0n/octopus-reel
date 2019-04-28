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

/* The constants used in common between the acquisition backend and daemon.
   Since this file is included both from kernel space backend C, and the
   frontend C++ routines, common convention is preferred for the definitions.*/

#ifndef OCTOPUS_ACQ_H
#define OCTOPUS_ACQ_H

/* FIFOs for frontend<->backend communication of acq. */
#define ACQ_F2BFIFO	(2)
#define ACQ_B2FFIFO	(3)

#define SAMPLE_RATE	(int)(250)  /* Soon, will (may) be dynamically adjustable */

/* Since we depend on amplifier CMRR, range is hard-coded as microvolts.
   but it is for this one to be given as a module parameter as well. */
#define AMP_RANGE	(200.) /* Amplifier range in uVpeak */

#define ACQ_PARBASE	(0x378)
#define ACQ_PARIRQ	(7)
#define ACQ_SERBASE	(0x3f8)
#define ACQ_SERIRQ	(4)

#endif
