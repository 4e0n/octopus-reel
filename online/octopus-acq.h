/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

/* The constants used in common between the acquisition backend and daemon.
   Since this file is included both from kernel space backend C, and the
   frontend C++ routines, common convention is preferred for the definitions.*/

#ifndef OCTOPUS_ACQ_H
#define OCTOPUS_ACQ_H

/* FIFOs for frontend<->backend communication of acq. */
#define ACQ_F2BFIFO	(2)
#define ACQ_B2FFIFO	(3)

#endif
