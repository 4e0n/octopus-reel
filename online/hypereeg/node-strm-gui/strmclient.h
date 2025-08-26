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

/* a.k.a. The EEG "Stream Client"..
    This is the master class who starts all others,
    e.g. predefined parameters, configuration and management class
    shared over all other classes. */

#ifndef STRMCLIENT_H
#define STRMCLIENT_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QVector>
#include "../globals.h"
#include "../tcpsample.h"
#include "../cmd_acqd.h"
#include "../cmd_strm.h"
#include "confparam.h"
#include "configparser.h"
#include "chninfo.h"
#include "strmcontrolwindow.h"
#include "ampstreamwindow.h"

const int HYPEREEG_ACQ_CLIENT_VER=200;
const int EEGFRAME_REFRESH_RATE=200; // Base refresh rate

const QString cfgPath=basePath+"strmclient.conf";

class StrmClient: public QObject {
 Q_OBJECT
 public:
  explicit StrmClient(QObject *parent=nullptr) : QObject(parent) {
   // Upstream (we're client)
   conf.commSocket=new QTcpSocket(this);
   conf.strmSocket=new QTcpSocket(this);
   conf.cmodSocket=new QTcpSocket(this);
   // Downstream (we're server)
   connect(&commServer,&QTcpServer::newConnection,this,&StrmClient::onNewCommClient);
   connect(&strmServer,&QTcpServer::newConnection,this,&StrmClient::onNewStrmClient);
   connect(&cmodServer,&QTcpServer::newConnection,this,&StrmClient::onNewCmodClient);
  }

  bool initialize() {
   QString commResponse; QStringList sList,sList2;
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "hnode_strm_gui: <ServerIP> is" << conf.ipAddr;
     qInfo() << "hnode_strm_gui: <Comm> listening on ports (comm,strm,cmod):" << conf.commPort << conf.strmPort << conf.cmodPort;
     qInfo() << "hnode_strm_gui: <GUI> Ctrl (X,Y,W,H):" << conf.guiCtrlX << conf.guiCtrlY << conf.guiCtrlW << conf.guiCtrlH;
     qInfo() << "hnode_strm_gui: <GUI> Strm (X,Y,W,H):" << conf.guiStrmX << conf.guiStrmY << conf.guiStrmW << conf.guiStrmH;

     // Setup command socket
     conf.commSocket->connectToHost(conf.ipAddr,conf.commPort); conf.commSocket->waitForConnected();

     commResponse=conf.commandToDaemon(CMD_ACQD_GETCONF);
     if (!commResponse.isEmpty()) qDebug() << "hnode_strm_gui: <Config> Daemon replied:" << commResponse;
     else qDebug() << "hnode_strm_gui: <Config> No response or timeout.";
     sList=commResponse.split(",");
     conf.init(sList[0].toInt()); // ampCount
     conf.sampleRate=sList[1].toInt();
     conf.tcpBufSize*=conf.sampleRate; // Convert tcpBufSize from seconds to samples
     conf.halfTcpBufSize=conf.tcpBufSize/2; // for fast-population check
     conf.tcpBuffer.resize(conf.tcpBufSize);

     conf.eegScrollDivider=conf.eegScrollCoeff[0];
     conf.eegScrollFrameTimeMs=conf.sampleRate/conf.eegScrollRefreshRate; // (1000sps/50Hz)=20ms=20samples
     // # of data to wait for, to be available for screen plot/scroller
     conf.scrAvailableSamples=conf.eegScrollFrameTimeMs*(conf.sampleRate/1000); // 20ms -> 20 sample

     conf.refChnCount=sList[2].toInt();
     conf.bipChnCount=sList[3].toInt();
     conf.refGain=sList[4].toFloat();
     conf.bipGain=sList[5].toFloat();
     conf.eegProbeMsecs=sList[6].toInt(); // This determines the maximum data feed rate together with sampleRate

     commResponse=conf.commandToDaemon(CMD_ACQD_GETCHAN);
     if (!commResponse.isEmpty()) qDebug() << "hnode_strm_gui: <Config> Daemon replied:" << commResponse;
     else qDebug() << "hnode_strm_gui: <Config> No response or timeout.";
     sList=commResponse.split("\n"); ChnInfo chn;
     for (int chnIdx=0;chnIdx<sList.size();chnIdx++) {
      sList2=sList[chnIdx].split(",");
      chn.physChn=sList2[0].toInt();
      chn.chnName=sList2[1];
      chn.topoTheta=sList2[2].toFloat();
      chn.topoPhi=sList2[3].toFloat();
      chn.isBipolar=(bool)sList2[4].toInt();
      conf.chns.append(chn);
     }
     conf.chnCount=conf.chns.size();
     conf.s0.resize(conf.scrAvailableSamples);
     conf.s0s.resize(conf.scrAvailableSamples);
     conf.s1.resize(conf.scrAvailableSamples);
     conf.s1s.resize(conf.scrAvailableSamples);
     for (unsigned int idx=0;idx<conf.scrAvailableSamples;idx++) {
      conf.s0[idx].resize(conf.chnCount); conf.s0s[idx].resize(conf.chnCount);
      conf.s1[idx].resize(conf.chnCount); conf.s1s[idx].resize(conf.chnCount);
     }

     for (auto& chn:conf.chns) qDebug() << chn.physChn << chn.chnName << chn.topoTheta << chn.topoPhi << chn.isBipolar;

     // Generate control window and EEG streaming windows
     controlWindow=new StrmControlWindow(&conf); controlWindow->show();

     conf.guiPending.resize(conf.ampCount);
     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
      conf.guiPending[ampIdx]=false;
     }

     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
      AmpStreamWindow* sWin=new AmpStreamWindow(ampIdx,&conf); sWin->show();
      ampStreamWindows.append(sWin);
     }

     connect(conf.strmSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onStrmDataReady);

     // At this point the scrolling widgets, and everything should be initialized and ready.
     // Setup data socket -- only safe after handshake and receiving crucial info about streaming
     conf.strmSocket->connectToHost(conf.ipAddr,conf.strmPort); conf.strmSocket->waitForConnected();
     conf.cmodSocket->connectToHost(conf.ipAddr,conf.cmodPort); conf.cmodSocket->waitForConnected();

     if (!commServer.listen(QHostAddress::Any,conf.svrCommPort)) {
      qCritical() << "hnode_strm_gui: Cannot start TCP server on <Comm> port:" << conf.svrCommPort;
      return true;
     }
     qInfo() << "hnode_strm_gui: <Comm> listening on port" << conf.svrCommPort;

     if (!strmServer.listen(QHostAddress::Any,conf.svrStrmPort)) {
      qCritical() << "hnode_strm_gui: Cannot start TCP server on <EEGData> port:" << conf.svrStrmPort;
      return true;
     }
     qInfo() << "hnode_strm_gui: <EEGData> listening on port" << conf.svrStrmPort;

     if (!cmodServer.listen(QHostAddress::Any,conf.svrCmodPort)) {
      qCritical() << "hnode_strm_gui: Cannot start TCP server on <CMData> port:" << conf.svrCmodPort;
      return true;
     }
     qInfo() << "hnode_strm_gui: <CMData> listening on port" << conf.svrCmodPort;

     return false;
    } else {
     qWarning() << "hnode_strm_gui: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "hnode_strm_gui: The config file" << cfgPath << "does not exist!";
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
    qInfo() << "hnode_strm_gui: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; //int iCmd=0; int iParam=0xffff;
   QIntValidator trigV(256,65535,this);
   qInfo() << "hnode_strm_gui: <Comm> Received command:" << cmd;
   if (!cmd.contains("|")) {
    sList.append(cmd);
    if (cmd==CMD_STRM_ACQINFO) {
/*     qDebug("hnode_strm_gui: <Comm> Sending Amplifier(s) Info..");
     client->write("-> EEG Samplerate: "+QString::number(conf.eegRate).toUtf8()+"sps\n");
     client->write("-> CM Samplerate: "+QString::number(conf.cmRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf.refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf.bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(conf.physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(conf.totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(conf.totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(conf.eegProbeMsecs).toUtf8()+"\n"); */
    } else if (cmd==CMD_STRM_AMPSYNC) {
    } else if (cmd==CMD_STRM_STATUS) {
     qDebug("hnode_strm_gui: <Comm> Sending visualization status..");
     client->write("EEG data scrolling.\n");
    } else if (cmd==CMD_STRM_DISCONNECT) {
//     qDebug("hnode_strm_gui: <Comm> Disconnecting client..");
//     client->write("Disconnecting...\n");
//     client->disconnectFromHost();
    } else if (cmd==CMD_STRM_GETCONF) {
/*     qDebug("hnode_strm_gui: <Comm> Sending Config Parameters..");
     client->write(QString::number(conf.ampCount).toUtf8()+","+ \
                   QString::number(conf.eegRate).toUtf8()+","+ \
                   QString::number(conf.cmRate).toUtf8()+","+ \
                   QString::number(conf.refChnCount).toUtf8()+","+ \
                   QString::number(conf.bipChnCount).toUtf8()+","+ \
                   QString::number(conf.refGain).toUtf8()+","+ \
                   QString::number(conf.bipGain).toUtf8()+","+ \
                   QString::number(conf.eegProbeMsecs).toUtf8()+"\n"); */
    } else if (cmd==CMD_STRM_GETCHAN) {
/*     qDebug("hnode_strm_gui: <Comm> Sending Channels' Parameters..");
     for (const auto& ch:chnInfo)
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.isBipolar).toUtf8()+"\n"); */
    } else if (cmd==CMD_STRM_REBOOT) {
//     qDebug("hnode_strm_gui: <Comm> Rebooting server (if privileges are enough)..");
//     client->write("Rebooting system (if privileges are enough)...\n");
//     system("/sbin/shutdown -r now");
    } else if (cmd==CMD_STRM_SHUTDOWN) {
//     qDebug("hnode_strm_gui: <Comm> Shutting down server (if privileges are enough)..");
//     client->write("Shutting down system (if privileges are enough)...\n");
//     system("/sbin/shutdown -h now");
    }
   } //else { // command with parameter
    //sList=cmd.split("|");
    //if (sList[0]=="TRIGGER") {
    // int pos=0;
    // if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
    //} // else {
//     qDebug("hnode_strm_gui: <Comm> Unknown command received..");
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
     qDebug() << "hnode_strm_gui: <EEGData> client from" << client->peerAddress().toString() << "disconnected.";
     strmClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "hnode_strm_gui: <EEGData> client connected from" << client->peerAddress().toString();
   }
  }

  void onNewCmodClient() {
   while (cmodServer.hasPendingConnections()) {
    QTcpSocket *client=cmodServer.nextPendingConnection(); cmodClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     qDebug() << "hnode_strm_gui: <CMData> client from" << client->peerAddress().toString() << "disconnected.";
     cmodClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "hnode_strm_gui: <CMData> client connected from" << client->peerAddress().toString();
   }
  }

  void drainAndBroadcast() {
//   TcpSample eegSample(conf.ampCount,conf.physChnCount);
//   TcpCMArray cmArray(conf.ampCount,conf.physChnCount);
//   //TcpCMArray cm2(conf.ampCount,conf.physChnCount);
//   while (acqThread->popEEGSample(&eegSample)) { QByteArray payLoad=eegSample.serialize();
//    //qDebug() << "Daemon: ampCount:" << tcpSample.amp.size() 
//    //     << " ChCount:" << tcpSample.amp[0].dataF.size() 
//    //     << " FirstVal:" << tcpSample.amp[0].dataF[0];
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
//   while (acqThread->popCMArray(&cmArray)) { QByteArray payLoad=cmArray.serialize();
//    // serialization-deserialization loopback test
//    //cm2.deserialize(payLoad,conf.physChnCount);
//    //qDebug() << cm2.cmLevel[1][3];
//    for (QTcpSocket *client:cmodClients) {
//     if (client->state()==QAbstractSocket::ConnectedState) {
//      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
//      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
//      sizeStream<<msgLength;
//      client->write(payLoad);
//      client->flush(); // write actual serialized block
//      //qDebug() << "CM sent!";
//     }
//    }
//   }
  }

 private:
  QTcpServer commServer,strmServer,cmodServer;
  QVector<QTcpSocket*> strmClients,cmodClients;
  StrmControlWindow *controlWindow; QVector<AmpStreamWindow*> ampStreamWindows;
};

#endif
