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

/* This is the generalized N-channel GUI client designed for recording
   continuous EEG with events, online averaging w/ threshold rejection w/
   per-channel configurable threshold levels. On a separate OpenGL window
   electrode positions may be measured over the connected digitizer system
   in a position invariant way (see the separate README.digitizer file for
   detailed information). The common-mode line voltage (50Hz) levels and the
   online averages  of channels are three-dimensionally shown on the same
   window, over the realistic head, on-the-fly.

   Upon execution, the file "octopus.cfg", residing in the same directory with
   the application, is parsed for overriding the default parameters,
   then the connections with ACQ and STIM daemons, and the Polhemus 3D
   digitizer is established using the relevant information.

   ACQ and STIM servers may either be some dedicated RTAI Linux systems, or
   merely be running on the same machine with the recorder..
   Surely, the overhead should be considered appropriately, in specialized
   laboratory designations.

   After successful parsing of octopus.cfg file, datachunks containing the
   [Chns' Acq data + Chns' Line noise levels + STIM/RESP event info] start
   to stream continuously from the pre-connected ACQ daemon over TCP/IP
   network. */

#include "acqmaster.h"
#include "acqcontrol.h"
#include "acqclient.h"

int main(int argc,char** argv) { AcqClient *acqClient;
 QApplication app(argc,argv); AcqMaster *acqM=new AcqMaster(&app);
 AcqControl *acqControl=new AcqControl(acqM); acqControl->show();
 for (unsigned int i=0;i<acqM->getAmpCount();i++) { acqClient=new AcqClient(acqM,i); acqClient->show(); }
 acqM->acqSendCommand(CS_ACQ_MANUAL_TRIG,AMP_SIMU_TRIG,0,0);
 return app.exec();
}
