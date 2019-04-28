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

#ifndef STIMDAEMON_H
#define STIMDAEMON_H

#include <QtNetwork>
#include <QThread>
#include <QVector>
#include <unistd.h>
#include "../octopus-stim.h"
#include "../../common/fb_command.h"
#include "../../common/cs_command.h"
#include "../patt_datagram.h"

class StimDaemon : public QTcpServer {
 Q_OBJECT
 public:
  StimDaemon(QObject *parent,QCoreApplication *app,
             QString h,int comm,int data,
             int fbf,int bff,char *shmb): QTcpServer(parent) {
   application=app; fbFifo=fbf; bfFifo=bff; shmBuffer=shmb;

   QHostAddress hostAddress(h);
   qDebug() << hostAddress.toString();

   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),
	   this,SLOT(slotIncomingCommand()));
   connect(this,SIGNAL(newConnection()),this,SLOT(slotIncomingData()));
   commandServer->setMaxPendingConnections(1);
   setMaxPendingConnections(1);

   if (!commandServer->listen(hostAddress,comm) ||
       !listen(hostAddress,data)) {
    qDebug(
     "octopus-stim-daemon: Error starting command and/or data server(s)!");
    application->quit();
   } else {
    qDebug(
     "octopus-stim-daemon: Servers started.. Waiting for client connection..");
   }
  }

  void f2bWrite(unsigned short code,int p0,int p1,int p2,int p3) {
   fbMsg.id=code; fbMsg.iparam[0]=p0; fbMsg.iparam[1]=p1;
   fbMsg.iparam[2]=p2; fbMsg.iparam[3]=p3;
   write(fbFifo,&fbMsg,sizeof(fb_command));
  }

 public slots:
  void slotIncomingCommand() {
   if ((commandSocket=commandServer->nextPendingConnection()))
    connect(commandSocket,SIGNAL(readyRead()),this,SLOT(slotHandleCommand()));
  }

  void slotHandleCommand() {
   QDataStream commandStream(commandSocket);
   if (commandSocket->bytesAvailable() >= sizeof(cs_command)) {
    commandStream.readRawData((char*)(&csCmd),sizeof(cs_command));
    qDebug("octopus-stim-daemon: Command received -> 0x%x(%d,%d)",
           csCmd.cmd,csCmd.iparam[0],csCmd.iparam[1]);
    switch (csCmd.cmd) {
     case CS_STIM_SET_PARADIGM:
      qDebug("octopus-stim-daemon: Paradigm changed to %d.",
             csCmd.iparam[0]);
      f2bWrite(STIM_SET_PARADIGM,csCmd.iparam[0],csCmd.iparam[1],0,0);
      break;

     case CS_STIM_LOAD_PATTERN_SYN:
      fileSize=csCmd.iparam[0]; csCmd.cmd=CS_STIM_LOAD_PATTERN_ACK;
      qDebug("octopus-stim-daemon: Starting pattern stream. FileSize=%d",
             fileSize);
      commandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command));
      commandSocket->flush();
      incomingDataSize=0;
      // Inform backend that data will come
      f2bWrite(STIM_LOAD_PATTERN,fileSize,0,0,0); // Inform for data is coming.
      break;

     case CS_STIM_START:
      qDebug("octopus-stim-daemon: Received start stim command.");
      f2bWrite(ACT_START,0,0,0,0);
      break;

     case CS_STIM_STOP:
      qDebug("octopus-stim-daemon: Received stop stim command.");
      f2bWrite(ACT_STOP,0,0,0,0);
      break;

     case CS_STIM_PAUSE:
      qDebug("octopus-stim-daemon: Received pause stim command.");
      f2bWrite(ACT_PAUSE,0,0,0,0);
      break;

     case CS_TRIG_START:
      qDebug("octopus-stim-daemon: Received start trigger command.");
      f2bWrite(STIM_TRIG_START,0,0,0,0);
      break;

     case CS_TRIG_STOP:
      qDebug("octopus-stim-daemon: Received stop trigger command.");
      f2bWrite(STIM_TRIG_STOP,0,0,0,0);
      break;

     case CS_STIM_SYNTHETIC_EVENT:
      qDebug("octopus-stim-daemon: Received synthetic event.");
      f2bWrite(STIM_SYNTH_EVENT,csCmd.iparam[0],0,0,0);
      break;

     case CS_STIM_SET_IIDITD_T1:
      f2bWrite(STIM_SET_IIDITD_T1,csCmd.iparam[0],0,0,0);
      break;

     case CS_STIM_SET_IIDITD_T2:
      f2bWrite(STIM_SET_IIDITD_T2,csCmd.iparam[0],0,0,0);
      break;

     case CS_STIM_SET_IIDITD_T3:
      f2bWrite(STIM_SET_IIDITD_T3,csCmd.iparam[0],0,0,0);
      break;

     case CS_REBOOT:
      qDebug("octopus-stim-daemon: System rebooting..");
      system("/sbin/shutdown -r now");
      commandSocket->close(); break;
      break;

     case CS_SHUTDOWN:
      qDebug("octopus-stim-daemon: System shutting down..");
      system("/sbin/shutdown -h now");
      commandSocket->close(); break;
     default:
      break;
    }
    commandSocket->close();
   }
  }

  void slotIncomingData() {
   if ((dataSocket=this->nextPendingConnection()))
    connect(dataSocket,SIGNAL(readyRead()),this,SLOT(slotReadData()));
  }

  void slotReadData() {
   // Incoming pattern data
   QDataStream dataStream(dataSocket);
//   dataStream.setVersion(QDataStream::Qt_4_0);
   while (dataSocket->bytesAvailable() >= sizeof(patt_datagram)) {
    dataStream.readRawData((char*)(&pattDatagram),sizeof(patt_datagram));
    incomingDataSize+=pattDatagram.size;

//    qDebug("octopus-stim-daemon: MN= 0x%x, %d, %d, %d",
//           pattDatagram.magic_number,
//           pattDatagram.size,
//           incomingDataSize,
//           fileSize);

    // Put the data over SHM transfer buffer..
    for (int i=0;i<(int)pattDatagram.size;i++)
     shmBuffer[i]=pattDatagram.data[i];

    // Send SYN(count) -- Inform backend
    fbMsg.id=STIM_PATT_XFER_SYN; fbMsg.iparam[0]=pattDatagram.size;
    write(fbFifo,&fbMsg,sizeof(fb_command));
    // Wait for ACK
    read(bfFifo,&bfMsg,sizeof(fb_command));

    if (incomingDataSize==fileSize)
     qDebug("octopus-stim-daemon: %d data elements transferred succesfully.",
            incomingDataSize);
   }
  }

 private:
  QCoreApplication *application;
  QTcpServer *commandServer; QTcpSocket *commandSocket,*dataSocket;
  cs_command csCmd; fb_command fbMsg,bfMsg;
  int fbFifo,bfFifo; char *shmBuffer;
  int fileSize,incomingDataSize;
  patt_datagram pattDatagram;
};

#endif
