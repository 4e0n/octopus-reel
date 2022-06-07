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

/* This is the stimulation 'frontend' daemon which takes orders from the
   relevant TCP port and conveys the incoming cmd to the kernelspace backend. */

#include <QApplication>
#include <QtCore>
#include <QIntValidator>
#include <stdlib.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>

#include "../octopus-stim.h"
#include "../fb_command.h"
#include "../cs_command.h"
#include "stimdaemon.h"

int main(int argc,char *argv[]) {
 int fbFifo,bfFifo; char *xferBuffer; fb_command reset_msg;
 QString host,comm,data; int commP,dataP;

 if (argc!=4) {
  qDebug("Usage: octopus-stim-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 host=QString::fromLatin1(argv[1]);
 comm=QString::fromLatin1(argv[2]);
 data=QString::fromLatin1(argv[3]);
 QIntValidator intValidator(1024,65535,NULL); int pos=0;
 if (intValidator.validate(comm,pos) != QValidator::Acceptable ||
     intValidator.validate(data,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 1024 and 65535 as ports.");
  qDebug("Usage: octopus-stim-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }
 commP=comm.toInt(); dataP=data.toInt();
 if (commP==dataP) {
  qDebug("Communication and Data ports should be different.");
  qDebug("Usage: octopus-stim-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 // Validation of also the hostname/ip would be better here..

 if ((fbFifo=open("/dev/rtf0",O_WRONLY))<0) {
  qDebug("octopus-stim-daemon: Cannot open f2b FIFO!");
  return -1;
 }

 if ((bfFifo=open("/dev/rtf1",O_RDONLY|O_NONBLOCK))<0) {
  qDebug("octopus-stim-daemon: Cannot open b2f FIFO!");
  return -1;
 }

 // Send RESET Backend Command
 reset_msg.id=STIM_RST_SYN;
 write(fbFifo,&reset_msg,sizeof(fb_command)); sleep(1); reset_msg.id=0;
 read(bfFifo,&reset_msg,sizeof(fb_command)); // Received ACK?

 if (reset_msg.id!=STIM_RST_ACK) {
  qDebug("octopus-stim-daemon: Kernel-space backend does not respond!");
  return -1;
 }

 // Acknowledge Signal came from backend.. Everything's OK.
 //  We may close and reopen FIFO in blocking mode...
 close(bfFifo); bfFifo=open("/dev/rtf1",O_RDONLY);

 if ((xferBuffer=(char *)rtai_malloc('XFER',XFERBUFSIZE)) == 0) {
  qDebug("octopus-stim-daemon: Kernel-space backend SHM could not be opened!");
  return -1;
 }

 QCoreApplication app(argc,argv);
 StimDaemon stimDaemon(0,&app,host,commP,dataP,fbFifo,bfFifo,xferBuffer);
 return app.exec();
}
