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

#ifndef HACQDAEMON_H
#define HACQDAEMON_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QIntValidator>
#include "../hacqglobals.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "../tcpcmarray.h"
#include "../tcpcommand.h"
#include "confparam.h"
#include "configparser.h"
#include "hacqthread.h"

const QString cfgPath="/etc/octopus_hacqd.conf";

class HAcqDaemon : public QObject {
 Q_OBJECT
 public:
  explicit HAcqDaemon(QObject *parent=nullptr) : QObject(parent) {
   connect(&commServer,&QTcpServer::newConnection,this,&HAcqDaemon::onNewCommClient);
   connect(&eegServer, &QTcpServer::newConnection,this,&HAcqDaemon::onNewEEGDataClient);
   connect(&cmServer, &QTcpServer::newConnection,this,&HAcqDaemon::onNewCMDataClient);
  }

  bool initialize() {
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf,&chnInfo)) {
#ifdef HACQ_VERBOSE
     qInfo() << "octopus_hacqd: Detailed channels info:";
     qInfo() << "--------------------------------------";
     for (const auto& ch:chnInfo)
      qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] Bipolar=%d",ch.physChn,qUtf8Printable(ch.chnName),
                                                            ch.topoTheta,ch.topoPhi,ch.topoX,ch.topoY,ch.isBipolar);
#endif
     conf.refChnMaxCount=REF_CHN_MAXCOUNT;
     conf.bipChnMaxCount=BIP_CHN_MAXCOUNT;
     conf.physChnCount=conf.refChnCount+conf.bipChnCount;
     conf.totalChnCount=conf.physChnCount+2;
     conf.totalCount=conf.ampCount*conf.totalChnCount;

     qInfo() << "---------------------------------------------------------------";
     qInfo() << "octopus_hacqd: ---> Datahandling Info <---";
     qInfo() << "octopus_hacqd: Connected # of amplifier(s):" << conf.ampCount;
     qInfo() << "octopus_hacqd: Sample Rate->" << conf.eegRate << "sps, CM Rate->" << conf.cmRate << "sps";
     qInfo() << "octopus_hacqd: TCP Ringbuffer allocated for" << conf.tcpBufSize << "seconds.";
     qInfo() << "octopus_hacqd: EEG data fetched every" << conf.eegProbeMsecs << "ms.";
     qInfo("octopus_hacqd: Per-amp Physical Channel#: %d (%d+%d)",conf.physChnCount,conf.refChnCount,conf.bipChnCount);
     qInfo() << "octopus_hacqd: Per-amp Total Channel# (with Trig and Offset):" << conf.totalChnCount;
     qInfo() << "octopus_hacqd: Total Channel# from all amps:" << conf.totalCount;
     qInfo() << "octopus_hacqd: Referential channels gain:" << conf.refGain;
     qInfo() << "octopus_hacqd: Bipolar channels gain:" << conf.bipGain;
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "octopus_hacqd: <ServerIP> is" << conf.ipAddr;
 
     if (!commServer.listen(QHostAddress::Any,conf.commPort)) {
      qCritical() << "octopus_hacqd: Cannot start TCP server on <Comm> port:" << conf.commPort;
      return true;
     }
     qInfo() << "octopus_hacqd: <Comm> listening on port" << conf.commPort;

     if (!eegServer.listen(QHostAddress::Any,conf.eegDataPort)) {
      qCritical() << "octopus_hacqd: Cannot start TCP server on <EEGData> port:" << conf.eegDataPort;
      return true;
     }
     qInfo() << "octopus_hacqd: <EEGData> listening on port" << conf.eegDataPort;

     if (!cmServer.listen(QHostAddress::Any,conf.cmDataPort)) {
      qCritical() << "octopus_hacqd: Cannot start TCP server on <CMData> port:" << conf.cmDataPort;
      return true;
     }
     qInfo() << "octopus_hacqd: <CMData> listening on port" << conf.cmDataPort;

     hAcqThread=new HAcqThread(&conf,this);
     connect(hAcqThread,&HAcqThread::sendData,this,&HAcqDaemon::drainAndBroadcast);
     hAcqThread->start(QThread::HighestPriority);

     return false;
    }
    qWarning() << "octopus_hacqd: The config file" << cfgPath << "is corrupt!";
    return true;
   }
   qWarning() << "octopus_hacqd: The config file" << cfgPath << "does not exist!";
   return true;
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
    qInfo() << "octopus_hacqd: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; int iCmd=0; int iParam=0xffff;
   QIntValidator trigV(256,65535,this);
   qInfo() << "octopus_hacqd: <Comm> Received command:" << cmd;
   if (!cmd.contains("|")) {
    sList.append(cmd);
    if (cmd==CMD_ACQINFO) iCmd=CMD_ACQINFO_B;
    else if (cmd==CMD_AMPSYNC) iCmd=CMD_AMPSYNC_B;
    else if (cmd==CMD_STATUS) iCmd=CMD_STATUS_B;
    else if (cmd==CMD_DISCONNECT) iCmd=CMD_DISCONNECT_B;
    else if (cmd==CMD_GETCONF) iCmd=CMD_GETCONF_B;
    else if (cmd==CMD_GETCHAN) iCmd=CMD_GETCHAN_B;
    else if (cmd==CMD_REBOOT) iCmd=CMD_REBOOT_B;
    else if (cmd==CMD_SHUTDOWN) iCmd=CMD_SHUTDOWN_B;
   } else { // command with parameter
    sList=cmd.split("|");
    if (sList[0]=="TRIGGER") {
     iCmd=CMD_TRIGGER_B; int pos=0;
     if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
    }
   }
   switch (iCmd) {
    case CMD_ACQINFO_B: qDebug("octopus_hacqd: <Comm> Sending Amplifier(s) Info..");
     client->write("-> EEG Samplerate: "+QString::number(conf.eegRate).toUtf8()+"sps\n");
     client->write("-> CM Samplerate: "+QString::number(conf.cmRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf.refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf.bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(conf.physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(conf.totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(conf.totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
     break;
    case CMD_TRIGGER_B:
     if (iParam<0xffff) {
      qDebug("octopus_hacqd: <Comm> Conveying **non-hardware** trigger to amplifier(s).. TCode:%d",iParam);
      hAcqThread->sendTrigger(iParam);
      client->write("**Non-hardware** trigger conveyed to amps.\n");
     } else {
      qDebug() << "octopus_hacqd: ERROR!!! <Comm> **Non-hardware** trigger is not between designated interval (256,65535).";
      client->write("Error! **Non-hardware** Trigger should be between (256,65535). Trigger not conveyed.\n");
     }
     break;
    case CMD_AMPSYNC_B: qDebug("octopus_hacqd: <Comm> Conveying SYNC to amplifier(s)..");
     hAcqThread->sendTrigger(TRIG_AMPSYNC);
     client->write("SYNC conveyed to amps.\n");
     break;
    case CMD_STATUS_B: qDebug("octopus_hacqd: <Comm> Sending Amp(s) status..");
     client->write("Amp(s) streaming EEG.\n");
     break;
    case CMD_DISCONNECT_B: qDebug("octopus_hacqd: <Comm> Disconnecting client..");
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
     break;
    case CMD_GETCONF_B: qDebug("octopus_hacqd: <Comm> Sending Config Parameters..");
     client->write(QString::number(conf.ampCount).toUtf8()+","+ \
                   QString::number(conf.eegRate).toUtf8()+","+ \
                   QString::number(conf.cmRate).toUtf8()+","+ \
                   QString::number(conf.refChnCount).toUtf8()+","+ \
                   QString::number(conf.bipChnCount).toUtf8()+","+ \
                   QString::number(conf.refGain).toUtf8()+","+ \
                   QString::number(conf.bipGain).toUtf8()+","+ \
                   QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
     break;
    case CMD_GETCHAN_B: qDebug("octopus_hacqd: <Comm> Sending Channels' Parameters..");
     for (const auto& ch:chnInfo)
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.isBipolar).toUtf8()+"\n");
     break;
    case CMD_REBOOT_B: qDebug("octopus_hacqd: <Comm> Rebooting server (if privileges are enough)..");
     client->write("Rebooting system (if privileges are enough)...\n");
     system("/sbin/shutdown -r now");
     break;
    case CMD_SHUTDOWN_B: qDebug("octopus_hacqd: <Comm> Shutting down server (if privileges are enough)..");
     client->write("Shutting down system (if privileges are enough)...\n");
     system("/sbin/shutdown -h now");
     break;
    default: qDebug("octopus_hacqd: <Comm> Unknown command received..");
     client->write("Unknown command..\n");
     break;
   }
  }

  void onNewEEGDataClient() {
   while (eegServer.hasPendingConnections()) {
    QTcpSocket *client=eegServer.nextPendingConnection(); eegDataClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     //for (int i=eegDataClients.size()-1;i>=0;--i) { QTcpSocket *client=eegDataClients.at(i);
     // if (client->state()!=QAbstractSocket::ConnectedState) { eegDataClients.removeAt(i); client->deleteLater(); }
     //}
     qDebug() << "octopus_hacqd: <EEGData> client from" << client->peerAddress().toString() << "disconnected.";
     eegDataClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "octopus_hacqd: <EEGData> client connected from" << client->peerAddress().toString();
   }
  }

  void onNewCMDataClient() {
   while (cmServer.hasPendingConnections()) {
    QTcpSocket *client=cmServer.nextPendingConnection(); cmDataClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     qDebug() << "octopus_hacqd: <CMData> client from" << client->peerAddress().toString() << "disconnected.";
     cmDataClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "octopus_hacqd: <CMData> client connected from" << client->peerAddress().toString();
   }
  }

  void drainAndBroadcast() {
   TcpSample eegSample(conf.ampCount,conf.physChnCount);
   TcpCMArray cmArray(conf.ampCount,conf.physChnCount);
   //TcpCMArray cm2(conf.ampCount,conf.physChnCount);
   while (hAcqThread->popEEGSample(&eegSample)) { QByteArray payLoad=eegSample.serialize();
    //qDebug() << "Daemon: ampCount:" << tcpSample.amp.size() 
    //     << " ChCount:" << tcpSample.amp[0].dataF.size() 
    //     << " FirstVal:" << tcpSample.amp[0].dataF[0];
    for (QTcpSocket *client:eegDataClients) {
     if (client->state()==QAbstractSocket::ConnectedState) {
      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
      sizeStream<<msgLength;
      client->write(payLoad);
      client->flush(); // write actual serialized block
     }
    }
   }
   while (hAcqThread->popCMArray(&cmArray)) { QByteArray payLoad=cmArray.serialize();
    // serialization-deserialization loopback test
    //cm2.deserialize(payLoad,conf.physChnCount);
    //qDebug() << cm2.cmLevel[1][3];
    for (QTcpSocket *client:cmDataClients) {
     if (client->state()==QAbstractSocket::ConnectedState) {
      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
      sizeStream<<msgLength;
      client->write(payLoad);
      client->flush(); // write actual serialized block
      //qDebug() << "CM sent!";
     }
    }
   }
  }

 private:
  HAcqThread *hAcqThread; QVector<QTcpSocket*> eegDataClients,cmDataClients;
  QVector<ChnInfo> chnInfo; QTcpServer commServer,eegServer,cmServer;
};

#endif
