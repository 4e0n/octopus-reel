/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

// Simple serial port definition for use in our Polhemus digitizer class.

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include <QString>

typedef struct _serial_device { QString devname;
 int baudrate,databits,parity,par_on,stopbit; } serial_device;

#endif
