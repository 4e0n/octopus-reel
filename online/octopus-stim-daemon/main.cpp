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

/* This is the stimulation 'frontend' daemon which takes orders from the
   relevant TCP port and conveys the incoming cmd to the kernelspace backend. */

#include <QApplication>
#include <QtCore>
//#include <QIntValidator>
//#include <stdlib.h>
//#include <rtai_fifos.h>
//#include <rtai_shm.h>
//#include "../stimglobals.h"
//#include "../fb_command.h"
//#include "../cs_command.h"
#include "stimdaemon.h"

int main(int argc,char *argv[]) {
 QCoreApplication app(argc,argv);

 StimDaemon stimDaemon(0,&app);
 return app.exec();
}
