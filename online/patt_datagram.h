/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

/* Structure for the segmented transfer of stimulus patterns to STIM server */
/* over TCP/IP network. */ 

#ifndef PATT_DATAGRAM_H
#define PATT_DATAGRAM_H

#define CHUNKSIZE	(128)

typedef struct _patt_datagram {
 unsigned int magic_number,size; char data[CHUNKSIZE];
} patt_datagram;

#endif
