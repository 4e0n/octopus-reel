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
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* This is the acquisition 'frontend' daemon of Octopus-ReEL, which has been
   customized to be interfaced with Eemagine commercial EEG amps
   (i.e. assuming two 64-channel "ANTneuro eego mylab" amps connected via USB)
   Hence this acq daemon makes the initial setup and then puts itself into one of the
   two possible modes of (1) EEG acquisition and (2) Impedance measurement.
   EEG acquisition mode is the default in which (N+M+2)*2 channels of data
   (N: Referential EEG channel, M: Bipolar channel, +1 event chn, +1 sample order chn)
   is received via two separate streams within ACQ thread into a "TCPsample" circular buffer
   When some client (i.e. consumer) is connected, the data in circular buffer is forwarded
   The client, most likely, is a dedicated remote host responsible from several
   on-the-fly calculations/visualizations on the EEG data received over the network.
   a remote and dedicated workstation over TCP/IP network.
   Each dataset is prepended with a sync-marker (constant pi)
   to assure integrity.

   The daemons connectivity with the TCP thread is event driven;
   i.e. as soon as the socket connection is up, thread is instantiated and started,
   and the stream is reestablished, and data already being fetched from amps
   within the ACQ thread is sent to that client (otherwise data is leaked/lost over the
   size of the circular buffer).
   Similarly, when the connection is down, the associated TCP thread is disposed. */

#include <QApplication>
#include <QtCore>

//#define EEMAGINE

#ifdef EEMAGINE
#include <eemagine/sdk/wrapper.cc>
#endif

#include "../chninfo.h"
#include "acqdaemon.h"

int main(int argc,char *argv[]) {
 QCoreApplication app(argc,argv);

 chninfo chnInfo;
 chnInfo.sampleRate=1000;
 chnInfo.refChnCount=refChnCount; chnInfo.refChnMaxCount=64;
 chnInfo.bipChnCount=bipChnCount; chnInfo.bipChnMaxCount=24;
 chnInfo.physChnCount=physChnCount; chnInfo.totalChnCount=totalChnCount;
 chnInfo.totalCount=2*totalChnCount;
 chnInfo.probe_msecs=100; // 100ms probetime

 // *** BACKEND VALIDATION ***

 qDebug("Datahandling Info:\n");
 qDebug() << "Per-amp Physical Channel#: " << chnInfo.physChnCount << "(" \
	  << chnInfo.refChnCount << "+" << chnInfo.bipChnCount << ")";
 qDebug() << "Per-amp Total Channel# (with Trig and Offset): " << chnInfo.totalChnCount;
 qDebug() << "Total Channel# from all amps: " << chnInfo.totalCount;

 AcqDaemon acqDaemon(0,&app,&chnInfo);
 return app.exec();
}

