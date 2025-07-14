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

/* This is the HyperEEG-Acq-Daemon.. this code was before together with
 * the GUI window showing on-the-fly, the common mode noise levels.
 * Now it is separated into two parts. The first one is this, the acquisition
 * daemon acquiring from multiple commercial USB EEG amps in a syncronized fashion.
 * The other part, HyperEEG-Acq-GUI (and multiple other types of clients) can now connect to this over TCP
 * via the given IP:port to visualize CM-noise levels, which normally is expected to
 * run on the same computer, and is very useful for maintenance of subjects in
 * a real-time observation for EEG-derived sophisticated variables under known
 * conditions, such as speech, or music.
 */

#include <QCoreApplication>
#include "../hacqglobals.h"
#ifdef EEMAGINE
#include <eemagine/sdk/wrapper.cc>
#endif
#include "hacqdaemon.h"

int main(int argc,char *argv[]) {
 QCoreApplication app(argc,argv);
 HAcqDaemon hAcqDaemon;

 if (hAcqDaemon.initialize()) {
  qCritical("octopus_hacqd: <FatalError> Failed to initialize Octopus-ReEL EEG Hyperacquisition daemon.");
  return 1;
 }

 return app.exec();
}
