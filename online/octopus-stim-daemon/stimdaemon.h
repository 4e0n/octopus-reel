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
#include "../patt_datagram.h"
#include "../fb_command.h"
#include "../cs_command.h"
#include "../stim_test_para.h"

class StimDaemon : public QTcpServer {
 Q_OBJECT
 public:
  StimDaemon(QObject *parent,QCoreApplication *app,
             QString h,int comm,int data,
             int fbf,int bff,char *shmb): QTcpServer(parent) {
   application=app; fbFifo=fbf; bfFifo=bff; shmBuffer=shmb;
   QHostAddress hostAddress(h);

   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),this,SLOT(incomingCommand()));
   connect(this,SIGNAL(newConnection()),this,SLOT(incomingData()));
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
   fbWrite(STIM_SET_PARADIGM,PARA_ITD_OPPCHN2,0);
  }

  void fbWrite(unsigned short code,int p1,int p2) {
   fbMsg.id=code; fbMsg.iparam[0]=p1; fbMsg.iparam[1]=p2;
   write(fbFifo,&fbMsg,sizeof(fb_command));
  }

 public slots:
  void incomingCommand() {
   if ((commandSocket=commandServer->nextPendingConnection()))
    connect(commandSocket,SIGNAL(readyRead()),this,SLOT(readCommand()));
  }

  void readCommand() {
   QDataStream commandStream(commandSocket);
//   commandStream.setVersion(QDataStream::Qt_4_0);
   if (commandSocket->bytesAvailable() >= sizeof(cs_command)) {
    commandStream.readRawData((char*)(&csCommand),sizeof(cs_command));
    qDebug("octopus-stim-daemon: Command received -> 0x%x(%d,%d)",
           csCommand.cmd,csCommand.iparam[0],csCommand.iparam[1]);
    switch (csCommand.cmd) {
     case CS_STIM_SET_PARADIGM:
      qDebug("octopus-stim-daemon: Paradigm changed to %d.",
             csCommand.iparam[0]);
      fbWrite(STIM_SET_PARADIGM,csCommand.iparam[0],csCommand.iparam[1]);
      break;

     case CS_STIM_LOAD_PATTERN_SYN:
      fileSize=csCommand.iparam[0]; csCommand.cmd=CS_STIM_LOAD_PATTERN_ACK;
      qDebug("octopus-stim-daemon: Starting pattern stream. FileSize=%d",
             fileSize);
      commandStream.writeRawData((const char*)(&csCommand),sizeof(cs_command));
      commandSocket->flush();
      incomingDataSize=0;
      // Inform backend that data will come
      fbWrite(STIM_LOAD_PATTERN,fileSize,0); // Inform b.e that data is coming.
      break;

     case CS_STIM_START:
      qDebug("octopus-stim-daemon: Received start stim command.");
      fbWrite(STIM_START,0,0);
      break;

     case CS_STIM_STOP:
      qDebug("octopus-stim-daemon: Received stop stim command.");
      fbWrite(STIM_STOP,0,0);
      break;

     case CS_STIM_PAUSE:
      qDebug("octopus-stim-daemon: Received pause stim command.");
      fbWrite(STIM_PAUSE,0,0);
      break;

     case CS_STIM_RESUME:
      qDebug("octopus-stim-daemon: Received resume stim command.");
      fbWrite(STIM_RESUME,0,0);
      break;

     case CS_TRIG_START:
      qDebug("octopus-stim-daemon: Received start trigger command.");
      fbWrite(TRIG_START,0,0);
      break;

     case CS_TRIG_STOP:
      qDebug("octopus-stim-daemon: Received stop trigger command.");
      fbWrite(TRIG_STOP,0,0);
      break;

     case CS_STIM_LIGHTS_ON:
      qDebug("octopus-stim-daemon: Received lights on stim command.");
      fbWrite(STIM_LIGHTS_ON,0,0);
      break;

     case CS_STIM_LIGHTS_DIMM:
      qDebug("octopus-stim-daemon: Received lights dimm stim command.");
      fbWrite(STIM_LIGHTS_DIMM,0,0);
      break;

     case CS_STIM_LIGHTS_OFF:
      qDebug("octopus-stim-daemon: Received lights off stim command.");
      fbWrite(STIM_LIGHTS_OFF,0,0);
      break;

     case CS_STIM_SYNTHETIC_EVENT:
      qDebug("octopus-stim-daemon: Received synthetic event.");
      fbWrite(STIM_SYNTH_EVENT,csCommand.iparam[0],0);
      break;

     case CS_STIM_SET_PARAM_P1:
      fbWrite(STIM_SET_PARAM_P1,csCommand.iparam[0],0);
      break;

     case CS_STIM_SET_PARAM_P2:
      fbWrite(STIM_SET_PARAM_P2,csCommand.iparam[0],0);
      break;

     case CS_STIM_SET_PARAM_P3:
      fbWrite(STIM_SET_PARAM_P3,csCommand.iparam[0],0);
      break;

     case CS_STIM_SET_PARAM_P4:
      fbWrite(STIM_SET_PARAM_P4,csCommand.iparam[0],0);
      break;

     case CS_STIM_SET_PARAM_P5:
      fbWrite(STIM_SET_PARAM_P5,csCommand.iparam[0],0);
      break;

     case CS_REBOOT:
      qDebug("octopus-stim-daemon: System rebooting..");
      system("/sbin/reboot");
      break;

     case CS_SHUTDOWN:
      qDebug("octopus-stim-daemon: System shutting down..");
      system("/sbin/halt");
     default:
      break;
    }
    commandSocket->close();
   }
  }

  void incomingData() {
   if ((dataSocket=this->nextPendingConnection()))
    connect(dataSocket,SIGNAL(readyRead()),this,SLOT(readData()));
  }

  void readData() {
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
    fbMsg.id=STIM_XFER_SYN; fbMsg.iparam[0]=pattDatagram.size;
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
  QTcpServer *commandServer;
  QTcpSocket *commandSocket,*dataSocket;

  cs_command csCommand; fb_command fbMsg,bfMsg;
  int fbFifo,bfFifo; char *shmBuffer;

  int fileSize,incomingDataSize;
  patt_datagram pattDatagram;
};

#endif
