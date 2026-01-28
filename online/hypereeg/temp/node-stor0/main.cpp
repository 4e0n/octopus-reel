/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

/* This is the HyperEEG Stor (Daemon) Node.. Its duty is to be connect
 * to node-acq on startup like the other client nodes for EEG stream,
 * and according to the command its given by other clients, it can
 * record, organize, etc. the streaming EEG data in .eeg/.vmrk format.
 */

#include <QCoreApplication>
#include "../common/globals.h"
#include "stordaemon.h"

int main(int argc,char *argv[]) {
 QCoreApplication app(argc,argv);
 StorDaemon storDaemon;

 omp_diag();

 if (storDaemon.initialize()) {
  qCritical("node_stor: <FatalError> Failed to initialize Octopus-ReEL EEG Stor daemon node.");
  return 1;
 }

 return app.exec();
}
