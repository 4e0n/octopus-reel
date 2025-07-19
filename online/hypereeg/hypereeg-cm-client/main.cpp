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

/*
 * This is the HyperEEG "Common Mode" GUI Client, which is may be one of the
 * simplest client types in a Octopus-ReEL network.
 * It connects to the Hyper-acq daemon over TCP via a pair of sockets
 * (IP:commandPort:cmStreamPort) to visualize CM-noise levels.
 *
 * Even though it can live anywhere on the network, it most likely runs
 * on the same computer together with the daemon, for its GUI window being
 * visible to the EEG technician-experimenter to adjust any problematic
 * electrode connections during a session of real-time observation for
 * EEG-derived sophisticated variables computed and visualized
 * under determined conditions such as speech, or music.
 */

#include <QApplication>
#include "acqmaster.h"

int main(int argc,char* argv[]) {
 QApplication app(argc,argv);
 AcqMaster master(&app);
 return app.exec();
}
