/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

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

/* This is the acquisition 'frontend' daemon which takes (N+N+2)-channel data
   (N EEG channel + N Noise Level + 2 event channels) acquired in realtime
   kernel-space backend and gathered over SHM. Data as a whole, is forwarded
   to the connected GUI client preferably running on a dedicated workstation
   over TCP/IP network. Each set is prepended with a sync-marker (constant pi)
   to assure integrity.

   The daemons connectivity with the backend is event driven;
   As soon as the socket connection is up, START_ACQ command is sent to
   kernel-space backend, and in the same way, when connection is down,
   backend acquisition is stopped and the data at hand is disposed. */

#include <QApplication>
#include <QtCore>
#include <QIntValidator>
#include <QHostInfo>
#include <stdlib.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>

#include "../acq.h"
#include "../fb_command.h"

static fb_command cmd;

int f2b_cmd(int fbf,int bff,int command) {
 cmd.id=ACQ_CMD_F2B;
 cmd.iparam[0]=command; cmd.iparam[1]=cmd.iparam[2]=0;
 write(fbf,&cmd,sizeof(fb_command)); sleep(1);
 read(bff,&cmd,sizeof(fb_command)); // result..
 if (!(cmd.id==ACQ_CMD_B2F)) {
  qDebug("octopus-acq-daemon: Kernel-space backend does not respond!");
  return -100;
 }
 return cmd.iparam[0];
}

void reset_cmd() { for (int i=0;i<4;i++) { cmd.iparam[i]=0; } }

#include "acqdaemon.h"

int main(int argc,char *argv[]) {
 int fbFifo,bfFifo,shmBufSize,totalCount,sampleRate; float *shmBuffer;
 QString host,comm,data,dStr1,dStr2; int commP,dataP;

 // *** COMMAND LINE VALIDATION ***

 if (argc!=4) {
  qDebug("Usage: octopus-acq-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 QHostInfo qhi=QHostInfo::fromName(QString::fromLatin1(argv[1]));
 host=qhi.addresses().first().toString();
 comm=QString::fromLatin1(argv[2]);
 data=QString::fromLatin1(argv[3]);
 QIntValidator intValidator(1024,65535,NULL); int pos=0;
 if (intValidator.validate(comm,pos) != QValidator::Acceptable ||
     intValidator.validate(data,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 1024 and 65535 as ports.");
  qDebug("Usage: octopus-acq-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 commP=comm.toInt(); dataP=data.toInt();
 if (commP==dataP) {
  qDebug("Communication and Data ports should be different.");
  qDebug("Usage: octopus-acq-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 // Validation of also the hostname/ip would be better here..

 // *** FIFO VALIDATION ***

 dStr2.setNum(ACQ_F2BFIFO); dStr1="/dev/rtf"+dStr2;
 if ((fbFifo=open(dStr1.toLatin1().data(),O_WRONLY))<0) {
  qDebug("octopus-acq-daemon: Cannot open f2b FIFO!"); return -1;
 }
 dStr2.setNum(ACQ_B2FFIFO); dStr1="/dev/rtf"+dStr2;
 if ((bfFifo=open(dStr1.toLatin1().data(),O_RDONLY|O_NONBLOCK))<0) {
  qDebug("octopus-acq-daemon: Cannot open b2f FIFO!"); return -1;
 }

 // *** BACKEND VALIDATION ***

 reset_cmd();
 if (f2b_cmd(fbFifo,bfFifo,F2B_RESET_SYN)!=B2F_RESET_ACK) { // Received ACK?
  qDebug("octopus-acq-daemon: Kernel-space could not be reset!"); return -1;
 } else {
  shmBufSize=cmd.iparam[1]; totalCount=cmd.iparam[2]; sampleRate=cmd.iparam[3];
  qDebug("Backend Info:\n");
  qDebug("SHMBufSize=%d, TotalCount=%d, SampleRate=%d\n",
         shmBufSize,totalCount,sampleRate);
 }
 // Possibly validation of the returned values from backend..

 // *** CLOSE and REOPEN FIFO IN BLOCKING MODE ***

 close(bfFifo); bfFifo=open(dStr1.toLatin1().data(),O_RDONLY);

 // *** SHM/ACQBUF VALIDATION ***

 if ((shmBuffer=(float *)rtai_malloc('BUFF',
                          shmBufSize*totalCount*sizeof(float)))==0) {
  qDebug("octopus-acq-daemon: Kernel-space backend SHM could not be opened!");
  return -1;
 } else {
  qDebug("octopus-acq-daemon: SHM address=%p",shmBuffer);
 }

 QCoreApplication app(argc,argv);
 AcqDaemon acqDaemon(0,&app,host,commP,dataP,
                     shmBufSize,totalCount,sampleRate,fbFifo,bfFifo,shmBuffer);
 return app.exec();
}
