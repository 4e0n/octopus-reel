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

/* This is the HyperEEG Acq (Daemon) Node.. in other words the "master node"
 * this code was before together with the GUI window, showing on-the-fly,
 * the common mode noise levels.
 * Now it is separated into two parts. The acquisition daemon acquiring
 * from multiple (USB) EEG amps, and stereo audio@48Ksps from the the
 * alsa2-default audio input device, in a syncronized fashion.
 *
 * The second separated part, HyperEEG-CM-GUI (multiple other types of clients
 * connect the same way) can now connect to this daemon over TCP via and an
 * established socket set (IP:commandPort:eegStreamPort:cmStreamPort) to
 * visualize CM-noise levels, which normally/practically is assumed to run
 * on the same computer, for being visible to the EEG technician-experimenter
 * to adjust any problematic electrode connections during a session of real-time
 * observation for EEG-derived sophisticated variables computed and visualized
 * under determined conditions such as speech, or music.
 */

#include <QCoreApplication>

#include "../globals.h"

#ifdef EEMAGINE
#include <eemagine/sdk/wrapper.cc>
#endif

#include "acqdaemon.h"

int main(int argc,char *argv[]) {
 QCoreApplication app(argc,argv);
 AcqDaemon acqDaemon;

 omp_diag();

 if (acqDaemon.initialize()) {
  qCritical("hnode_acqd: <FatalError> Failed to initialize Octopus-ReEL EEG Hyperacquisition daemon node.");
  return 1;
 }

 return app.exec();
}
