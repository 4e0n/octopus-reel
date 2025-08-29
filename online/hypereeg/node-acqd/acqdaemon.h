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

#ifndef ACQDAEMON_H
#define ACQDAEMON_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QIntValidator>
#include <QDateTime>
#include "../globals.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "../tcpcmarray.h"
#include "../cmd_acqd.h"
#include "confparam.h"
#include "configparser.h"
#include "acqthread.h"

const int HYPEREEG_ACQ_DAEMON_VER=200;

const QString cfgPath=hyperConfPath+"node-master-acqd.conf";

class AcqDaemon : public QObject {
 Q_OBJECT
 public:
  explicit AcqDaemon(QObject *parent=nullptr) : QObject(parent) {
   connect(&commServer,&QTcpServer::newConnection,this,&AcqDaemon::onNewCommClient);
   connect(&strmServer,&QTcpServer::newConnection,this,&AcqDaemon::onNewStrmClient);
   connect(&cmodServer,&QTcpServer::newConnection,this,&AcqDaemon::onNewCmodClient);
  }

  bool initialize() {
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf,&chnInfo)) {
#ifdef HACQ_VERBOSE
     qInfo() << "hnode_acqd: Detailed channels info:";
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

     conf.eegSamplesInTick=conf.eegRate*conf.eegProbeMsecs/1000;

     conf.dumpRaw=false;

     qInfo() << "---------------------------------------------------------------";
     qInfo() << "hnode_acqd: ---> Datahandling Info <---";
     qInfo() << "hnode_acqd: Connected # of amplifier(s):" << conf.ampCount;
     qInfo() << "hnode_acqd: Sample Rate->" << conf.eegRate << "sps, CM Rate->" << conf.cmRate << "sps";
     qInfo() << "hnode_acqd: TCP Ringbuffer allocated for" << conf.tcpBufSize << "seconds.";
     qInfo() << "hnode_acqd: EEG data fetched every" << conf.eegProbeMsecs << "ms.";
     qInfo("hnode_acqd: Per-amp Physical Channel#: %d (%d+%d)",conf.physChnCount,conf.refChnCount,conf.bipChnCount);
     qInfo() << "hnode_acqd: Per-amp Total Channel# (with Trig and Offset):" << conf.totalChnCount;
     qInfo() << "hnode_acqd: Total Channel# from all amps:" << conf.totalCount;
     qInfo() << "hnode_acqd: Referential channels gain:" << conf.refGain;
     qInfo() << "hnode_acqd: Bipolar channels gain:" << conf.bipGain;
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "hnode_acqd: <ServerIP> is" << conf.ipAddr;
 
     if (!commServer.listen(QHostAddress::Any,conf.commPort)) {
      qCritical() << "hnode_acqd: Cannot start TCP server on <Comm> port:" << conf.commPort;
      return true;
     }
     qInfo() << "hnode_acqd: <Comm> listening on port" << conf.commPort;

     if (!strmServer.listen(QHostAddress::Any,conf.strmPort)) {
      qCritical() << "hnode_acqd: Cannot start TCP server on <EEGData> port:" << conf.strmPort;
      return true;
     }
     qInfo() << "hnode_acqd: <EEGData> listening on port" << conf.strmPort;

     if (!cmodServer.listen(QHostAddress::Any,conf.cmodPort)) {
      qCritical() << "hnode_acqd: Cannot start TCP server on <CMData> port:" << conf.cmodPort;
      return true;
     }
     qInfo() << "hnode_acqd: <CMData> listening on port" << conf.cmodPort;

     acqThread=new AcqThread(&conf,this);
     connect(acqThread,&AcqThread::sendData,this,&AcqDaemon::drainAndBroadcast);
     acqThread->start(QThread::HighestPriority);

     return false;
    }
    qWarning() << "hnode_acqd: The config file" << cfgPath << "is corrupt!";
    return true;
   }
   qWarning() << "hnode_acqd: The config file" << cfgPath << "does not exist!";
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
    qInfo() << "hnode_acqd: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; int iParam=0xffff;
   QIntValidator trigV(256,65535,this);
   qInfo() << "hnode_acqd: <Comm> Received command:" << cmd;
   if (!cmd.contains("|")) {
    sList.append(cmd);
    if (cmd==CMD_ACQD_ACQINFO) {
     qDebug("hnode_acqd: <Comm> Sending Amplifier(s) Info..");
     client->write("-> EEG Samplerate: "+QString::number(conf.eegRate).toUtf8()+"sps\n");
     client->write("-> CM Samplerate: "+QString::number(conf.cmRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf.refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf.bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(conf.physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(conf.totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(conf.totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
    } else if (cmd==CMD_ACQD_AMPSYNC) {
     qDebug("hnode_acqd: <Comm> Conveying SYNC to amplifier(s)..");
     acqThread->sendTrigger(TRIG_AMPSYNC);
     client->write("SYNC conveyed to amps.\n");
    } else if (cmd==CMD_ACQD_STATUS) {
     qDebug("hnode_acqd: <Comm> Sending Amp(s) status..");
     client->write("Amp(s) streaming EEG.\n");
    } else if (cmd==CMD_ACQD_DISCONNECT) { 
     qDebug("hnode_acqd: <Comm> Disconnecting client..");
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
    } else if (cmd==CMD_ACQD_GETCONF) {
     qDebug("hnode_acqd: <Comm> Sending Config Parameters..");
     client->write(QString::number(conf.ampCount).toUtf8()+","+ \
                   QString::number(conf.eegRate).toUtf8()+","+ \
                   QString::number(conf.cmRate).toUtf8()+","+ \
                   QString::number(conf.refChnCount).toUtf8()+","+ \
                   QString::number(conf.bipChnCount).toUtf8()+","+ \
                   QString::number(conf.refGain).toUtf8()+","+ \
                   QString::number(conf.bipGain).toUtf8()+","+ \
                   QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
    } else if (cmd==CMD_ACQD_GETCHAN) {
     qDebug("hnode_acqd: <Comm> Sending Channels' Parameters..");
     for (const auto& ch:chnInfo)
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.isBipolar).toUtf8()+"\n");
    } else if (cmd==CMD_ACQD_DUMPRAWON) {
     static constexpr char dumpSign[]="OCTOPUS_HEEG"; // 12 bytes, no \0
     QDateTime currentDT(QDateTime::currentDateTime());
     QString tStamp=currentDT.toString("yyyyMMdd-hhmmss-zzz");
     conf.hEEGFile.setFileName("/opt/octopus/heegdump-"+tStamp+".raw");
     conf.hEEGFile.open(QIODevice::WriteOnly);
     conf.hEEGStream.setDevice(&(conf.hEEGFile));
     conf.hEEGStream.setVersion(QDataStream::Qt_5_15); // 5_12
     conf.hEEGStream.setByteOrder(QDataStream::LittleEndian);
     conf.hEEGStream.setFloatingPointPrecision(QDataStream::SinglePrecision); // 32-bit
     conf.hEEGStream.writeRawData(dumpSign,sizeof(dumpSign)-1); // write signature *without* length or NUL
     conf.hEEGStream << quint32(conf.ampCount) << quint32(conf.physChnCount);
     conf.dumpRaw=true;
     client->write("hnode_acqd: Raw EEG dumping started.\n");
    } else if (cmd==CMD_ACQD_DUMPRAWOFF) {
     conf.dumpRaw=false; conf.hEEGStream.setDevice(nullptr); conf.hEEGFile.close();
     client->write("hnode_acqd: Raw EEG dumping stopped.\n");
    } else if (cmd==CMD_ACQD_REBOOT) {
     qDebug("hnode_acqd: <Comm> Rebooting server (if privileges are enough)..");
     client->write("Rebooting system (if privileges are enough)...\n");
     system("/sbin/shutdown -r now");
    } else if (cmd==CMD_ACQD_SHUTDOWN) {
     qDebug("hnode_acqd: <Comm> Shutting down server (if privileges are enough)..");
     client->write("Shutting down system (if privileges are enough)...\n");
     system("/sbin/shutdown -h now");
    }
   } else { // command with parameter
    sList=cmd.split("|");
    if (sList[0]=="TRIGGER") {
     int pos=0;
     if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
     if (iParam<0xffff) {
      qDebug("hnode_acqd: <Comm> Conveying **non-hardware** trigger to amplifier(s).. TCode:%d",iParam);
      acqThread->sendTrigger(iParam);
      client->write("**Non-hardware** trigger conveyed to amps.\n");
     } else {
      qDebug() << "hnode_acqd: ERROR!!! <Comm> **Non-hardware** trigger is not between designated interval (256,65535).";
      client->write("Error! **Non-hardware** Trigger should be between (256,65535). Trigger not conveyed.\n");
     }
    }
    //else {
    // qDebug("hnode_acqd: <Comm> Unknown command received..");
    // client->write("Unknown command..\n");
    //}
   }
  }

  void onNewStrmClient() {
   while (strmServer.hasPendingConnections()) {
    QTcpSocket *client=strmServer.nextPendingConnection();
    
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);   // TCP_NODELAY
    // optional: smaller buffers to avoid deep OS queues
    client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 64*1024);
    
    strmClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     //for (int i=strmClients.size()-1;i>=0;--i) { QTcpSocket *client=strmClients.at(i);
     // if (client->state()!=QAbstractSocket::ConnectedState) { strmClients.removeAt(i); client->deleteLater(); }
     //}
     qDebug() << "hnode_acqd: <EEGData> client from" << client->peerAddress().toString() << "disconnected.";
     strmClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "hnode_acqd: <EEGData> client connected from" << client->peerAddress().toString();
   }
  }

  void onNewCmodClient() {
   while (cmodServer.hasPendingConnections()) {
    QTcpSocket *client=cmodServer.nextPendingConnection();
    
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);   // TCP_NODELAY
    // optional: smaller buffers to avoid deep OS queues
    client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 64*1024);
    
    cmodClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     qDebug() << "hnode_acqd: <CMData> client from" << client->peerAddress().toString() << "disconnected.";
     cmodClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "hnode_acqd: <CMData> client connected from" << client->peerAddress().toString();
   }
  }

  void drainAndBroadcast() {
   TcpSample eegSample(conf.ampCount,conf.physChnCount); TcpCMArray cmArray(conf.ampCount,conf.physChnCount);

   while (acqThread->popEEGSample(&eegSample)) { QByteArray payLoad=eegSample.serialize();
    for (QTcpSocket *client:strmClients) {
     if (client->state()==QAbstractSocket::ConnectedState) {
      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
      sizeStream << msgLength;
      client->write(payLoad);
      client->flush(); // write actual serialized block
     }
    }
   }

   while (acqThread->popCMArray(&cmArray)) { QByteArray payLoad=cmArray.serialize();
    for (QTcpSocket *client:cmodClients) {
     if (client->state()==QAbstractSocket::ConnectedState) {
      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
      sizeStream << msgLength;
      client->write(payLoad);
      client->flush(); // write actual serialized block
     }
    }
   }

  }

 private:
  QTcpServer commServer,strmServer,cmodServer;
  QVector<QTcpSocket*> strmClients,cmodClients;
  AcqThread *acqThread; QVector<ChnInfo> chnInfo;
};

#endif
