/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QVector>
#include <QFile>
#include <QDataStream>
#include <QDir>
#include "../common/version.h"
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
#include "recthread.h"

const QString cfgPathX=confPath+"hypereeg.conf";
const QString RECROOTDIR="/opt/octopus/stor/heeg";

class StorDaemon: public QObject {
 Q_OBJECT
 public:
  explicit StorDaemon(QObject *parent=nullptr) : QObject(parent) {
   // We're client
   conf.acqCommSocket=new QTcpSocket(this);
   conf.acqCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   conf.acqStrmSocket=new QTcpSocket(this);
   conf.acqStrmSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqStrmSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // We're server
   connect(&storCommServer,&QTcpServer::newConnection,this,&StorDaemon::onNewCommClient);
  }

  bool initialize() { QString cfgPath;
   if (cfgPathX=="~") cfgPath=QDir::homePath();
   if (cfgPathX.startsWith("~/")) cfgPath=QDir::homePath()+cfgPathX.mid(1);
   if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {

     // Constants or calculated global settings upon the ones read from config file
     conf.tcpBuffer=QVector<TcpSample>(conf.tcpBufSize,TcpSample(conf.ampCount,conf.chnCount));

     qInfo() << "---------------------------------------------------------------";
     qInfo() << "<ServerIP> is" << conf.acqIpAddr;
     qInfo() << "<Comm> listening on ports (comm,strm):" << conf.acqCommPort << conf.acqStrmPort;

     // ---------------------------------------------------------------------------------------------------------------
     if (!storCommServer.listen(QHostAddress::Any,conf.storCommPort)) {
      qCritical() << "<Comm> Cannot start TCP server on <Comm> port:" << conf.storCommPort;
      return true;
     }
     qInfo() << "<Comm> listening on port" << conf.storCommPort;
     // ---------------------------------------------------------------------------------------------------------------

     // Setup data socket -- only safe after handshake and receiving crucial info about streaming
     connect(conf.acqStrmSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onStrmDataReady); // TCP handler for instream

     // Connect for streaming data -- only safe after handshake and receiving crucial info about streaming
     conf.acqStrmSocket->connectToHost(conf.acqIpAddr,conf.acqStrmPort); conf.acqStrmSocket->waitForConnected();

     // Threads should be here
     recThread=new RecThread(&conf,this);
     recThread->start(QThread::HighestPriority);

     return false;
    } else {
     qWarning() << "<Config> The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "<Config> The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

 private slots:
  void onNewCommClient() {
   while (storCommServer.hasPendingConnections()) {
    QTcpSocket *client=storCommServer.nextPendingConnection();
    //client->setSocketOption(QAbstractSocket::LowDelayOption,1);
    //client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray cmd=client->readAll().trimmed();
     handleCommand(QString::fromUtf8(cmd),client);
    });
    connect(client,&QTcpSocket::disconnected,client,&QObject::deleteLater);
    qInfo() << "<Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList;
   qInfo() << "<Comm> Received command:" << cmd;
   if (!cmd.contains("=")) {
    sList.append(cmd);
    if (cmd==CMD_STOR_STATUS) {
     qInfo() << "<Comm> Sending Storage Daemon status..";
     client->write("node-stor: Storage server ready.\n");
    } else if (cmd==CMD_STOR_REC_ON) {
     qInfo() << "<Comm> Recording on..";

     const QString tStamp=QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss-zzz");
//     const QString base="/opt/octopus/data/" + tStamp;

     const QString rootDir=RECROOTDIR;
     const QString recDir=rootDir+"/"+tStamp;

     QDir dir;
     if (!dir.mkpath(recDir)) {
      qCritical() << "<Storage> cannot create directory:" << recDir;
      return; // or handle error
     }

     const QString base=recDir+"/data";

     recThread->requestStartRecording(base); // direct call OK (it sets atomic flag)
     client->write("node-stor: RECSTART accepted.\n");
    } else if (cmd==CMD_STOR_REC_OFF) {
     qInfo() << "<Comm> Recording off..";
     recThread->requestStopRecording();
     client->write("node-stor: RECSTOP accepted.\n");
    }
   }
  }

 private:
  QTcpServer storCommServer; RecThread *recThread;
};
