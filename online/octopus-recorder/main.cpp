/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

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

#include "octopus_rec_master.h"
#include "octopus_recorder.h"

int main(int argc,char** argv) {
 QApplication app(argc,argv); RecMaster *p=new RecMaster(&app);
 if (!p->initSuccess) {
  qDebug("Octopus-Recorder: Initialization failed.. exiting.."); return -1;
 } else { Recorder recorder(p); recorder.show(); return app.exec(); }
}
