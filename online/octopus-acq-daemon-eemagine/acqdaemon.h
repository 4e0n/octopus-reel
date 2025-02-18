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
#include "tcpthread.h"

class AcqDaemon : public QTcpServer {
 Q_OBJECT
 public:
  AcqDaemon(QObject *parent,QCoreApplication *app,chninfo *c): QTcpServer(parent) {
   application=app; chnInfo=c;

   // Parse system config file for variables
   QStringList cfgValidLines,opts,opts2,bufSection,netSection;
   QFile cfgFile; QTextStream cfgStream;
   QString cfgLine; QStringList cfgLines; cfgFile.setFileName("/etc/octopus_acqd.conf");
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_acqd: <.conf> cannot load /etc/octopus_acqd.conf.";
    qDebug() << "octopus_acqd: <.conf> Falling back to hardcoded defaults.";
    confTcpBufSize=10; confHost="127.0.0.1";  confCommP=65002;  confDataP=65003;
   } else { cfgStream.setDevice(&cfgFile);
    while (!cfgStream.atEnd()) { cfgLine=cfgStream.readLine(160); // Max Line Size
     cfgLines.append(cfgLine); } cfgFile.close();

    // Parse config
    for (int i=0;i<cfgLines.size();i++) { // Isolate valid lines
     if (!(cfgLines[i].at(0)=='#') &&
         cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]); }

    for (int i=0;i<cfgValidLines.size();i++) {
     opts=cfgValidLines[i].split("|");
          if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else { qDebug() << "octopus_acqd: <.conf> Unknown section in .conf file!";
      app->quit();
     }
    }

    // BUF
    if (bufSection.size()>0) {
     for (int i=0;i<bufSection.size();i++) { opts=bufSection[i].split("=");
      if (opts[0].trimmed()=="PAST") { confTcpBufSize=opts[1].toInt();
       if (!(confTcpBufSize >= 2 && confTcpBufSize <= 50)) {
        qDebug() << "octopus_acqd: <.conf> BUF|PAST not within inclusive (2,50) seconds range!";
        app->quit();
       }
      }
     }
    } else { confTcpBufSize=5000; }

    // NET
    if (netSection.size()>0) {
     for (int i=0;i<netSection.size();i++) { opts=netSection[i].split("=");

      if (opts[0].trimmed()=="ACQ") { opts2=opts[1].split(",");
       if (opts2.size()==3) { confHost=opts2[0].trimmed();
        QHostInfo qhiAcq=QHostInfo::fromName(confHost);
        confHost=qhiAcq.addresses().first().toString();
        qDebug() << "octopus_acqd: <.conf> (this) Host IP is " << confHost;
        confCommP=opts2[1].toInt(); confDataP=opts2[2].toInt();
        // Simple port validation..
        if ((!(confCommP >= 1024 && confCommP <= 65535)) ||
            (!(confDataP >= 1024 && confDataP <= 65535))) {
         qDebug() << "octopus_acqd: <.conf> Error in Hostname/IP and/or port settings!";
         app->quit();
        }
       }
      } else {
       qDebug() << "octopus_acqd: <.conf> Parse error in Hostname/IP(v4) Address!";
       app->quit();
      }
     }
    } else {
     confHost="127.0.0.1";  confCommP=65002;  confDataP=65003;
    }
   }

   tcpBuffer.resize(confTcpBufSize*chnInfo->sampleRate); // in seconds, after which data is lost.

   QHostAddress hostAddress(confHost);
   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),this,SLOT(slotIncomingCommand()));
   setMaxPendingConnections(1);

   if (!commandServer->listen(hostAddress,confCommP) || !listen(hostAddress,confDataP)) {
    qDebug() << "octopus_acqd: Error starting command and/or data server(s)!";
    application->quit();
   } else {
    qDebug() << "octopus_acqd: Daemon started successfully..";
    qDebug() << "octopus_acqd: Waiting for client connection..";
   }

   tcpBuffer.resize(confTcpBufSize*chnInfo->sampleRate); tcpBufPIdx=tcpBufCIdx=0;

   eegImpedanceMode=false;
   acqThread=new AcqThread(this,chnInfo,&tcpBuffer,&tcpBufPIdx,&tcpBufCIdx,&daemonRunning,&eegImpedanceMode,&mutex);
   //connect(acqThread,SIGNAL(finished()),acqThread,SLOT(deleteLater()));
   acqThread->start(QThread::HighestPriority);
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
     case CS_ACQ_SETMODE:
		       if (csCmd.iparam[0]==0) eegImpedanceMode=true;
		       else eegImpedanceMode=false;
		       break;
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
   if (!clientConnected) {
    qDebug("octopus-acq-daemon: Incoming client connection..");
    if (!dataSocket.setSocketDescriptor(socketDescriptor)) {
     qDebug("octopus-acq-daemon: TCP Socket Error!"); clientConnected=false; return;
    } else {
     qDebug("octopus-acq-daemon:  ..accepted.. connecting..");
     connect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
     if (dataSocket.waitForConnected()) {
      //qDebug() << dataSocket.peerAddress();
      clientConnected=true;
      qDebug("octopus-acq-daemon: Client TCP connection established.");
      // Launch thread responsible for sending acq.data asynchronously..
      tcpThread=new TcpThread(this,&tcpBuffer,&tcpBufPIdx,&tcpBufCIdx,&daemonRunning,&clientConnected,&mutex);
      // Delete thread if communication breaks..
      connect(tcpThread,SIGNAL(finished()),tcpThread,SLOT(deleteLater()));
      tcpThread->start(QThread::HighestPriority);
     } else {
      qDebug("octopus-acq-daemon: Cannot connect to Client TCP socket!!!");
     }
    }
   } else qDebug("octopus-acq-daemon:  ..not accepted.. already connected..");
  }

 private slots:
  void slotDisconnected() {
   disconnect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
   clientConnected=false;
   while (tcpThread->isRunning()); // Wait for thread termination
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
  QCoreApplication *application; QTcpServer *commandServer;
  QTcpSocket *commandSocket; QTcpSocket dataSocket;
  QVector<tcpsample> tcpBuffer; quint64 tcpBufPIdx,tcpBufCIdx;
  bool daemonRunning,eegImpedanceMode,clientConnected;

  AcqThread *acqThread; TcpThread *tcpThread; QMutex mutex;
  cs_command csCmd; chninfo *chnInfo;

  QString confHost; 
  int confTcpBufSize,confCommP,confDataP;
};

#endif
