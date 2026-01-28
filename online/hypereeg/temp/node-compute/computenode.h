/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

/* a.k.a. The HyperEEG "Compute Node"..
    This is the master compute node class who starts all others,
    e.g. predefined parameters, configuration and management class
    shared over all other classes. */

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QIntValidator>
#include <QString>
#include <QVector>
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
#include "chninfo.h"
#include "computethread.h"

const int HYPEREEG_COMPUTE_NODE_VER=200;
const int COMPUTE_REFRESH_RATE=200; // Base refresh rate

const QString cfgPath=hyperConfPath+"node-compute.conf";

class ComputeNode: public QObject {
 Q_OBJECT
 public:
  explicit ComputeNode(QObject *parent=nullptr) : QObject(parent) {
   // Upstream (we're client)
   conf.commSocket=new QTcpSocket(this);
   conf.strmSocket=new QTcpSocket(this);
   // Downstream (we're server)
   connect(&commServer,&QTcpServer::newConnection,this,&ComputeNode::onNewCommClient);
   connect(&strmServer,&QTcpServer::newConnection,this,&ComputeNode::onNewStrmClient);
  }

  bool initialize() { //QString commResponse; QStringList sList,sList2;
   if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "node_compute: <ServerIP> is" << conf.ipAddr;
     qInfo() << "node_compute: <Comm> listening on ports (comm,strm):" << conf.commPort << conf.strmPort;
     qInfo() << "node_compute: <GUI> Ctrl (X,Y,W,H):" << conf.guiCtrlX << conf.guiCtrlY << conf.guiCtrlW << conf.guiCtrlH;
     qInfo() << "node_compute: <GUI> Amp (X,Y,W,H):" << conf.guiAmpX << conf.guiAmpY << conf.guiAmpW << conf.guiAmpH;

     //controlWindow=new ControlWindow(&conf); controlWindow->show();

     // Generate EEG GUI (streaming/channel interpolation/3D head view) windows
     //for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
     // AmpWindow* sWin=new AmpWindow(ampIdx,&conf); ampWindows.append(sWin); sWin->show();
     //}
     connect(conf.strmSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onStrmDataReady);

     // ------------------------------------------------------------------------------------
     // At this point the scrolling widgets, and everything should be initialized and ready.

     // Setup data socket -- only safe after handshake and receiving crucial info about streaming
     conf.strmSocket->connectToHost(conf.ipAddr,conf.strmPort); conf.strmSocket->waitForConnected();
     qDebug() << "HERE";

     if (!commServer.listen(QHostAddress::Any,conf.svrCommPort)) {
      qCritical() << "node_compute: Cannot start TCP server on <Comm> port:" << conf.svrCommPort;
      return true;
     }
     qInfo() << "node_compute: <Comm> listening on port" << conf.svrCommPort;

     if (!strmServer.listen(QHostAddress::Any,conf.svrStrmPort)) {
      qCritical() << "node_compute: Cannot start TCP server on <EEGStream> port:" << conf.svrStrmPort;
      return true;
     }
     qInfo() << "node_compute: <EEGStream> listening on port" << conf.svrStrmPort;

     return false;
    } else {
     qWarning() << "node_compute: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "node_compute: The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

 signals:
  void sendData();

 private slots:
  void onNewCommClient() {
   while (commServer.hasPendingConnections()) {
    QTcpSocket *client=commServer.nextPendingConnection();
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray cmd=client->readAll().trimmed();
     handleCommand(QString::fromUtf8(cmd),client);
    });
    connect(client,&QTcpSocket::disconnected,client,&QObject::deleteLater);
    qInfo() << "node_compute: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; //int iCmd=0; int iParam=0xffff;
   QIntValidator trigV(256,65535,this);
   qInfo() << "node_compute: <Comm> Received command:" << cmd;
   if (!cmd.contains("|")) {
    sList.append(cmd);
    if (cmd==CMD_COMP_ACQINFO) {
/*     qDebug("node_compute: <Comm> Sending Amplifier(s) Info..");
     client->write("-> EEG Samplerate: "+QString::number(conf.eegRate).toUtf8()+"sps\n");
     client->write("-> CM Samplerate: "+QString::number(conf.cmRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf.refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf.bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(conf.physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(conf.totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(conf.totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(conf.eegProbeMsecs).toUtf8()+"\n"); */
    } else if (cmd==CMD_COMP_AMPSYNC) {
    } else if (cmd==CMD_COMP_STATUS) {
     qDebug("node_compute: <Comm> Sending computation status..");
     client->write("Hemispheric powers are computed on-the-fly for the  streaming EEG.\n");
    } else if (cmd==CMD_COMP_DISCONNECT) {
//     qDebug("node_compute: <Comm> Disconnecting client..");
//     client->write("Disconnecting...\n");
//     client->disconnectFromHost();
    } else if (cmd==CMD_COMP_GETCONF) {
/*     qDebug("node_compute: <Comm> Sending Config Parameters..");
     client->write(QString::number(conf.ampCount).toUtf8()+","+ \
                   QString::number(conf.eegRate).toUtf8()+","+ \
                   QString::number(conf.cmRate).toUtf8()+","+ \
                   QString::number(conf.refChnCount).toUtf8()+","+ \
                   QString::number(conf.bipChnCount).toUtf8()+","+ \
                   QString::number(conf.refGain).toUtf8()+","+ \
                   QString::number(conf.bipGain).toUtf8()+","+ \
                   QString::number(conf.eegProbeMsecs).toUtf8()+"\n"); */
    } else if (cmd==CMD_COMP_GETCHAN) {
/*     qDebug("node_compute: <Comm> Sending Channels' Parameters..");
     for (const auto& ch:chnInfo)
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.isBipolar).toUtf8()+"\n"); */
    } else if (cmd==CMD_COMP_REBOOT) {
//     qDebug("node_compute: <Comm> Rebooting server (if privileges are enough)..");
//     client->write("Rebooting system (if privileges are enough)...\n");
//     system("/sbin/shutdown -r now");
    } else if (cmd==CMD_COMP_SHUTDOWN) {
//     qDebug("node_compute: <Comm> Shutting down server (if privileges are enough)..");
//     client->write("Shutting down system (if privileges are enough)...\n");
//     system("/sbin/shutdown -h now");
    }
   } //else { // command with parameter
    //sList=cmd.split("|");
    //if (sList[0]=="TRIGGER") {
    // int pos=0;
    // if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
    //} // else {
//     qDebug("node_compute: <Comm> Unknown command received..");
//     client->write("Unknown command..\n");
//    }
//   }
  }

  void onNewStrmClient() {
   while (strmServer.hasPendingConnections()) {
    QTcpSocket *client=strmServer.nextPendingConnection(); strmClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     //for (int i=strmClients.size()-1;i>=0;--i) { QTcpSocket *client=strmClients.at(i);
     // if (client->state()!=QAbstractSocket::ConnectedState) { strmClients.removeAt(i); client->deleteLater(); }
     //}
     qDebug() << "node_compute: <EEGData> client from" << client->peerAddress().toString() << "disconnected.";
     strmClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "node_compute: <EEGData> client connected from" << client->peerAddress().toString();
   }
  }

  void drainAndBroadcast() {
//   TcpSample eegSample(conf.ampCount,conf.physChnCount);
//   while (acqThread->popEEGSample(&eegSample)) { QByteArray payLoad=eegSample.serialize();
//    for (QTcpSocket *client:strmClients) {
//     if (client->state()==QAbstractSocket::ConnectedState) {
//      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
//      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
//      sizeStream<<msgLength;
//      client->write(payLoad);
//      client->flush(); // write actual serialized block
//     }
//    }
//   }
  }

 private:
  QTcpServer commServer,strmServer;
  QVector<QTcpSocket*> strmClients;
};
