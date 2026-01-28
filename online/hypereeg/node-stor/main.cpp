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

/* This is the HyperEEG "File recording/storage operations" Node.
 * Its main purpose is to record the EEG+audio data broadcast by
 * the node-acq to disk. Starting and stopping of recording is held
 * by proper commands from node-time.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include "stordaemon.h"

int main(int argc,char* argv[]) {
 QCoreApplication app(argc,argv);
 StorDaemon storDaemon;

 omp_diag();

 if (storDaemon.initialize()) {
  qCritical("node-stor: <FatalError> Failed to initialize Octopus-ReEL EEG HyperAcquisition File recording/storage daemon.");
  return 1;
 }

 return app.exec();
}
