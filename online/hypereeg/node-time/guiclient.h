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
#include "../common/globals.h"
#include "../common/tcpsample_pp.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
//#include "guichninfo.h"
#include "controlwindow.h"
#include "ampwindow.h"

const int EEGFRAME_REFRESH_RATE=100; // Base refresh rate

const QString cfgPathX=confPath()+"hypereeg.conf";

class GUIClient: public QObject {
 Q_OBJECT
 public:
  explicit GUIClient(QObject *parent=nullptr) : QObject(parent) {
   // Upstream (we're client)
   conf.acqCommSocket=new QTcpSocket(this);
   conf.acqCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   conf.acqStrmSocket=new QTcpSocket(this);
   conf.acqStrmSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqStrmSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // Upstream (we're client)
   conf.storCommSocket=new QTcpSocket(this);
   conf.storCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.storCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // Downstream (we're server)
   //connect(&commServer,&QTcpServer::newConnection,this,&GUIClient::onNewCommClient);
   //connect(&strmServer,&QTcpServer::newConnection,this,&GUIClient::onNewStrmClient);
  }

  bool initialize() { QString cfgPath;
   if (cfgPathX=="~") cfgPath=QDir::homePath();
   if (cfgPathX.startsWith("~/")) cfgPath=QDir::homePath()+cfgPathX.mid(1);
   if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {

     // Constants or calculated global settings upon the ones read from config file

     qInfo() << "---------------------------------------------------------------";
     qInfo() << "node-time: <ServerIP> is" << conf.acqIpAddr;
     qInfo() << "node-time: <Comm> listening on AcqServ ports (comm,strm):" << conf.acqCommPort << conf.acqStrmPort;
     qInfo() << "node-time: <Comm> listening on StorServ ports (comm):" << conf.storCommPort;
     qInfo() << "node-time: <GUI> Ctrl (X,Y,W,H):" << conf.guiCtrlX << conf.guiCtrlY << conf.guiCtrlW << conf.guiCtrlH;
     qInfo() << "node-time: <GUI> Amp (X,Y,W,H):" << conf.guiAmpX << conf.guiAmpY << conf.guiAmpW << conf.guiAmpH;

     // ---------------------------------------------------------------------------------------------------------------
     //if (!commServer.listen(QHostAddress::Any,conf.timeCommPort)) {
     // qCritical() << "node-time: Cannot start TCP server on <Comm> port:" << conf.timeCommPort;
     // return true;
     //}
     //qInfo() << "node-time: <Comm> listening on port" << conf.timeCommPort;
     //if (!strmServer.listen(QHostAddress::Any,conf.timeStrmPort)) {
     // qCritical() << "node-time: Cannot start TCP server on <Strm> port:" << conf.timeStrmPort;
     // return true;
     //}
     //qInfo() << "node-time: <Strm> listening on port" << conf.timeStrmPort;
     // ---------------------------------------------------------------------------------------------------------------

     controlWindow=new ControlWindow(&conf); controlWindow->show(); // Control Window

     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) { // Dynamic windows - one for each amp
      AmpWindow* sWin=new AmpWindow(ampIdx,&conf); ampWindows.append(sWin); sWin->show();
     }

     connect(conf.acqStrmSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onStrmDataReady); // TCP handler for instream

     // Connect for streaming data -- only safe after handshake and receiving crucial info about streaming
     conf.acqStrmSocket->connectToHost(conf.acqIpAddr,conf.acqStrmPort); conf.acqStrmSocket->waitForConnected();

     return false;
    } else {
     qWarning() << "node-time: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "node-time: The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

// private slots:
//  void onNewCommClient() {
//   while (commServer.hasPendingConnections()) {
//    QTcpSocket *client=commServer.nextPendingConnection();
//    //client->setSocketOption(QAbstractSocket::LowDelayOption,1);
//    //client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
//    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
//     QByteArray cmd=client->readAll().trimmed();
//     handleCommand(QString::fromUtf8(cmd),client);
//    });
//    connect(client,&QTcpSocket::disconnected,client,&QObject::deleteLater);
//    qInfo() << "node-time: <Comm> Client connected from" << client->peerAddress().toString();
//   }
//  }

//  void handleCommand(const QString &cmd,QTcpSocket *client) {
//  }

//  void onNewStrmClient() {
//   while (strmServer.hasPendingConnections()) {
//    QTcpSocket *client=strmServer.nextPendingConnection();
    
//    client->setSocketOption(QAbstractSocket::LowDelayOption,1); // TCP_NODELAY
//    client->setSocketOption(QAbstractSocket::KeepAliveOption,1);
//    // optional: smaller buffers to avoid deep OS queues
//    client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,64*1024);

//    strmClients.append(client);
//    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
//     qDebug() << "node-time: <Strm> client from" << client->peerAddress().toString() << "disconnected.";
//     strmClients.removeAll(client);
//     client->deleteLater();
//    });
//    qInfo() << "node-time: <Strm> client connected from" << client->peerAddress().toString();
//   }
//  }

 private:
//  QTcpServer commServer,strmServer;
//  QVector<QTcpSocket*> strmClients;
  ControlWindow *controlWindow; QVector<AmpWindow*> ampWindows;
};
