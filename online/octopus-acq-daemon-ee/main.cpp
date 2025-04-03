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

/* This is the acquisition 'frontend' daemon of Octopus-ReEL, which has been
   customized to be interfaced with Eemagine commercial EEG amps within a hyperscanning
   context (i.e. assuming more than one 64-channel "ANTneuro eego mylab" amps connected
   via USB). Hence this acq daemon makes the initial setup and then puts itself into one
   of the two possible modes of (1) EEG acquisition and (2) Impedance measurement.
   EEG acquisition mode is the default in which (N+M+2)*2 channels of data
   (N: Referential EEG channel, M: Bipolar channel, +1 event chn, +1 sample order chn)
   is received via N independent streams within ACQ thread into a "TCPsample" circular
   buffer. When some remote client (i.e. consumer) is connected over TCP, the data in
   circular buffer begins to be forwarded. The client is most likely a dedicated remote
   host responsible from several on-the-fly computations/visualizations on the collective
   EEG data from multiple subjects. When the acquisition daemon is launched, a syncronized
   trigger 0xFF is sent to all amplifiers via their parallel ports using the customly
   designed Arduino (Sparkfun Pro Micro)-based trigger multiplexer, causing the amplifier(s)
   receiving the trigger earlier to wait for others, thus causing them to be syncronized
   once for all. Another issue of EEG amplifiers possibly having different clock paces
   has been experimented and observed for still being syncronized after half an hour for
   two connected amps.

   As the amplifiers are not with me all the time but I can code **anywhere**, I translated
   the class hierarchy and namespace of Eemagine to a simulated environment called "Eesynth"
   which transparently and synthetically generated synthetic EEG data over the same system
   calls and functions. One can enable it by commenting out the EEMAGINE line definition
   in ../acqglobals.h (which is the main header with global definitions; either simulated or real).
   
   Due to copyright reasons, the respective Eemagine library isn't included in that project.
*/

#ifdef EEMAGINE
#include <eemagine/sdk/wrapper.cc>
#endif

#include "acqdaemon.h"
#include "acqdaemongui.h"
#include "acqthread.h"

int main(int argc,char *argv[]) {
 QApplication app(argc,argv);
 AcqDaemon acqDaemon(&app);
 AcqThread acqThread(&acqDaemon);
 AcqDaemonGUI acqDaemonGUI(&acqDaemon); acqDaemonGUI.show();
 acqThread.start(QThread::HighestPriority);
 return app.exec();
}
