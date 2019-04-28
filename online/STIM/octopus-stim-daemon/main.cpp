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
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
#include <QHostInfo>
#include <stdlib.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>

#include "../octopus-stim.h"
#include "../../common/fb_command.h"
//#include "../cs_command.h"

static fb_command cmd;

int f2b_cmd(int fbf,int bff,int command) {
 cmd.id=CMD_F2B;
 cmd.iparam[0]=command; cmd.iparam[1]=cmd.iparam[2]=0;
 write(fbf,&cmd,sizeof(fb_command)); sleep(1);
 read(bff,&cmd,sizeof(fb_command)); // result..
 if (!(cmd.id==CMD_B2F)) {
  qDebug("octopus-acq-daemon: Kernel-space backend does not respond!");
  return -100;
 }
 return cmd.iparam[0];
}

void reset_cmd() { for (int i=0;i<4;i++) { cmd.iparam[i]=0; } }

#include "stimdaemon.h"

int main(int argc,char *argv[]) {
 int fbFifo,bfFifo; char *shmBuffer;
 QString host,comm,data,dStr1,dStr2; int commP,dataP;

 // *** COMMAND LINE VALIDATION ***

 if (argc!=4) {
  qDebug("Usage: octopus-stim-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 QHostInfo qhi=QHostInfo::fromName(QString::fromAscii(argv[1]));
 host=qhi.addresses().first().toString();
 comm=QString::fromAscii(argv[2]);
 data=QString::fromAscii(argv[3]);
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

 dStr2.setNum(STIM_F2BFIFO); dStr1="/dev/rtf"+dStr2;
 if ((fbFifo=open(dStr1.toAscii().data(),O_WRONLY))<0) {
  qDebug("octopus-stim-daemon: Cannot open f2b FIFO!"); return -1;
 }
 dStr2.setNum(STIM_B2FFIFO); dStr1="/dev/rtf"+dStr2;
 if ((bfFifo=open(dStr1.toAscii().data(),O_RDONLY|O_NONBLOCK))<0) {
  qDebug("octopus-stim-daemon: Cannot open b2f FIFO!"); return -1;
 }

 // *** BACKEND VALIDATIO N ***

 reset_cmd();
 if (f2b_cmd(fbFifo,bfFifo,F2B_RESET_SYN)!=B2F_RESET_ACK) { // Received ACK?
  qDebug("octopus-stim-daemon: Kernel-space backend does not respond!");
 } else {
  qDebug("Backend Info:\n");
  qDebug("HMBufSize=\n");
 }
 // Some validation of the returned values from backend..

 // *** CLOSE and REOPEN FIFO IN BLOCKING MODE ***
 // (Acknowledge Signal came from backend.. Everything's OK).
 
 close(bfFifo); bfFifo=open(dStr1.toAscii().data(),O_RDONLY);

 // *** SHM (Stimulus Upstream) Buffer Validation ***
 
 if ((shmBuffer=(char *)rtai_malloc('STMB',SHMBUFSIZE))==0) {
  qDebug("octopus-stim-daemon: Kernel-space backend SHM could not be opened!");
  return -1;
 } else {
  qDebug("octopus-stim-daemon: SHM address=%p",shmBuffer);
 }

 QCoreApplication app(argc,argv);
 StimDaemon stimDaemon(0,&app,host,commP,dataP,
		       fbFifo,bfFifo,shmBuffer);
 return app.exec();
}
