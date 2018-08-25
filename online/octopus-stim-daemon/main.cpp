/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

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
 int fbFifo,bfFifo; char *shmBuffer; fb_command reset_msg;
 QString host,comm,data; int commP,dataP;

 if (argc!=4) {
  qDebug("Usage: octopus-stim-daemon <hostname|ip> <comm port> <data port>");
  return -1;
 }

 host=QString::fromAscii(argv[1]);
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

 if ((shmBuffer=(char *)rtai_malloc('PATT',SHMBUFSIZE)) == 0) {
  qDebug("octopus-stim-daemon: Kernel-space backend SHM could not be opened!");
  return -1;
 }

 QCoreApplication app(argc,argv);
 StimDaemon stimDaemon(0,&app,host,commP,dataP,fbFifo,bfFifo,shmBuffer);
 return app.exec();
}


