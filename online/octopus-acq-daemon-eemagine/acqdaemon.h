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

#ifndef ACQDAEMON_H
#define ACQDAEMON_H

#include <QtNetwork>
#include <QThread>
#include <QMutex>
#include <QVector>
#include <unistd.h>

#include "../cs_command.h"
#include "../tcpsample.h"
#include "../chninfo.h"
#include "acqthread.h"

class AcqDaemon : public QTcpServer {
 Q_OBJECT
 public:
  AcqDaemon(QObject *parent,QCoreApplication *app,
            QString h,int comm,int data,chninfo *c): QTcpServer(parent) {
   application=app; chnInfo=c; tcpBuffer.resize(chnInfo->probe_msecs);

   // TCP Buffer for subsequent acq data.. extra one for sampleset checkmark..
   connected=false;

   QHostAddress hostAddress(h);

   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),this,SLOT(slotIncomingCommand()));
   setMaxPendingConnections(1);

   if (!commandServer->listen(hostAddress,comm) || !listen(hostAddress,data)) {
    qDebug("octopus-acq-daemon: Error starting command and/or data server(s)!");
    application->quit();
   } else {
    qDebug(
     "octopus-acq-daemon: Servers started.. Waiting for client connection..");
   }
  }

 public slots:
  void slotIncomingCommand() {
   if ((commandSocket=commandServer->nextPendingConnection()))
    connect(commandSocket,SIGNAL(readyRead()),this,SLOT(slotHandleCommand()));
  }

  void slotHandleCommand() {
   QDataStream commandStream(commandSocket);
   if ((quint64)commandSocket->bytesAvailable() >= sizeof(cs_command)) {
    commandStream.readRawData((char*)(&csCmd),sizeof(cs_command));
    qDebug("octopus-acq-daemon: Command received -> 0x%x.",csCmd.cmd);
    switch (csCmd.cmd) {
     case CS_ACQ_INFO: qDebug(
                        "octopus-acq-daemon: Sending Chn.Info..");
                       csCmd.cmd=CS_ACQ_INFO_RESULT;
		       csCmd.iparam[0]=chnInfo->sampleRate;
                       csCmd.iparam[1]=chnInfo->refChnCount;
                       csCmd.iparam[2]=chnInfo->bipChnCount;
                       csCmd.iparam[3]=chnInfo->physChnCount;
                       csCmd.iparam[4]=chnInfo->refChnMaxCount;
                       csCmd.iparam[5]=chnInfo->bipChnMaxCount;
                       csCmd.iparam[6]=chnInfo->physChnMaxCount;
                       csCmd.iparam[7]=chnInfo->totalChnCount;
                       csCmd.iparam[8]=chnInfo->totalCount;
                       csCmd.iparam[9]=chnInfo->probe_msecs;
                       commandStream.writeRawData((const char*)(&csCmd),
                                                  sizeof(cs_command));
                       //dataSocket.flush();
                       commandSocket->flush(); break;
     case CS_REBOOT:   qDebug("octopus-acq-daemon: System rebooting..");
                       system("/sbin/shutdown -r now"); commandSocket->close(); break;
     case CS_SHUTDOWN: qDebug("octopus-acq-daemon: System shutting down..");
                       system("/sbin/shutdown -h now"); commandSocket->close(); break;
     case CS_ACQ_TRIGTEST:
     default:          break;
    }
   }
  }

 protected:
  // Data port "connection handler"..
  void incomingConnection(qintptr socketDescriptor) {
   if (!connected) {
    qDebug("octopus-acq-daemon: Incoming client connection..");
    if (!dataSocket.setSocketDescriptor(socketDescriptor)) {
     qDebug("octopus-acq-daemon: TCP Socket Error!"); connected=false; return;
    } else {
     qDebug("octopus-acq-daemon:  ..accepted.. connecting..");
     connect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
     if (dataSocket.waitForConnected()) {
      //qDebug() << dataSocket.peerAddress();
      connected=true;
      qDebug("octopus-acq-daemon: Client TCP connection established.");
      // Launch thread responsible for sending acq.data asynchronously..
      acqThread=new AcqThread(this,&tcpBuffer,chnInfo,&connected,&mutex);
      // Delete thread if communication breaks..
      connect(acqThread,SIGNAL(finished()),acqThread,SLOT(deleteLater()));
      acqThread->start(QThread::HighestPriority);
     } else {
      qDebug("octopus-acq-daemon: Cannot connect to Client TCP socket!!!");
     }
    }
   } else qDebug("octopus-acq-daemon:  ..not accepted.. already connected..");
  }

 private slots:
  void slotDisconnected() {
   disconnect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
   connected=false;
   while (acqThread->isRunning()); // Wait for thread termination
   dataSocket.close();
   qDebug("octopus-acq-daemon: Client disconnected!");
  }

  void slotSendData() {
   QDataStream outputStream(&dataSocket);
   //qDebug() << outputStream.status();
   outputStream.writeRawData((const char*)((char *)(tcpBuffer.data())),
                             tcpBuffer.size()*sizeof(tcpsample));
   dataSocket.flush();
  }

 private:
  QCoreApplication *application; AcqThread *acqThread; QMutex mutex;
  QTcpServer *commandServer; QTcpSocket *commandSocket; QTcpSocket dataSocket;
  cs_command csCmd; chninfo *chnInfo;
  QVector<tcpsample> tcpBuffer; bool connected;
};

#endif
