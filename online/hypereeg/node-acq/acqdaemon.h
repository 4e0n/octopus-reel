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
#include <QIntValidator>
#include <QDateTime>
#include <QDir>
#include "../common/version.h"
#include "../common/globals.h"
#include "../common/sample.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
#include "acqthread.h"
#include "tcpthread.h"

const QString cfgPathX=confPath+"hypereeg.conf";

class AcqDaemon : public QObject {
 Q_OBJECT
 public:
  explicit AcqDaemon(QObject *parent=nullptr) : QObject(parent) {
   // Downstream (we're server)
   connect(&commServer,&QTcpServer::newConnection,this,&AcqDaemon::onNewCommClient);
   connect(&strmServer,&QTcpServer::newConnection,this,&AcqDaemon::onNewStrmClient);
  }

  bool initialize() { QString cfgPath;
   if (cfgPathX=="~") cfgPath=QDir::homePath();
   if (cfgPathX.startsWith("~/")) cfgPath=QDir::homePath()+cfgPathX.mid(1);
   if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {

     // Constants or calculated global settings upon the ones read from config file
     conf.refChnMaxCount=REF_CHN_MAXCOUNT; conf.bipChnMaxCount=BIP_CHN_MAXCOUNT;
     conf.physChnCount=conf.refChnCount+conf.bipChnCount; conf.totalChnCount=conf.physChnCount+2;
     conf.totalCount=conf.ampCount*conf.totalChnCount;
     conf.eegSamplesInTick=conf.eegRate*conf.eegProbeMsecs/1000;
     conf.dumpRaw=false;
//     conf.interList.resize(conf.ampCount);
//     conf.offList.resize(conf.ampCount);
//     conf.generateElecLists();

#ifdef HACQ_VERBOSE
     qInfo() << "node-acq: Detailed channels info:";
     qInfo() << "---------------------------------";
     QString interElec;
     for (const auto& ch:conf.chnInfo) {
      interElec=""; for (int i=0;i<ch.interElec.size();i++) interElec.append(QString::number(ch.interElec[i])+" ");
      qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] ChnType=%d, Inter=%s",ch.physChn,qUtf8Printable(ch.chnName),
                                                                      ch.topoTheta,ch.topoPhi,ch.topoX,ch.topoY,ch.type,
                                                                      interElec.toUtf8().constData());
     }
#endif
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "node-acq: ---> Datahandling Info <---";
     qInfo() << "node-acq: Connected # of amplifier(s):" << conf.ampCount;
     qInfo() << "node-acq: Sample Rate->" << conf.eegRate << "sps";
     qInfo() << "node-acq: TCP Ringbuffer allocated for" << conf.tcpBufSize << "seconds.";
     qInfo() << "node-acq: EEG data fetched every" << conf.eegProbeMsecs << "ms.";
     qInfo("node-acq: Per-amp Physical Channel#: %d (%d+%d)",conf.physChnCount,conf.refChnCount,conf.bipChnCount);
     qInfo() << "node-acq: Per-amp Total Channel# (with Trig and Offset):" << conf.totalChnCount;
     qInfo() << "node-acq: Total Channel# from all amps:" << conf.totalCount;
     qInfo() << "node-acq: Referential channels gain:" << conf.refGain;
     qInfo() << "node-acq: Bipolar channels gain:" << conf.bipGain;
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "node-acq: <ServerIP> is" << conf.acqIpAddr;

     // -----------------------------------------------------------------------------------------------------------------
     if (!commServer.listen(QHostAddress::Any,conf.acqCommPort)) {
      qCritical() << "node-acq: Cannot start TCP server on <Comm> port:" << conf.acqCommPort;
      return true;
     }
     qInfo() << "node-acq: <Comm> listening on port" << conf.acqCommPort;
     if (!strmServer.listen(QHostAddress::Any,conf.acqStrmPort)) {
      qCritical() << "node-acq: Cannot start TCP server on <Strm> port:" << conf.acqStrmPort;
      return true;
     }
     qInfo() << "node-acq: <Strm> listening on port" << conf.acqStrmPort;
     // -----------------------------------------------------------------------------------------------------------------

     conf.tcpBufSize*=conf.eegRate;
     conf.tcpBuffer=QVector<TcpSample>(conf.tcpBufSize,TcpSample(conf.ampCount,conf.physChnCount));

     acqThread=new AcqThread(&conf,this); // Thread for acquiring/packaging EEG data from amps+Audio data (Producer)
     tcpThread=new TcpThread(&conf,this); // Thread for serving the packages from ringbuffer to connected clients (Consumer)
     connect(tcpThread,&TcpThread::sendPacketSignal,this,&AcqDaemon::sendPacket,Qt::QueuedConnection); // Send packets timely
     acqThread->start(QThread::HighestPriority);
     tcpThread->start(QThread::HighestPriority);

     return false;
    } else {
     qWarning() << "node-acq: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "node-acq: The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

 private slots:
  void onNewCommClient() {
   while (commServer.hasPendingConnections()) {
    QTcpSocket *client=commServer.nextPendingConnection();
    //client->setSocketOption(QAbstractSocket::LowDelayOption,1);
    //client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,60*1024);
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray cmd=client->readAll().trimmed();
     handleCommand(QString::fromUtf8(cmd),client);
    });
    connect(client,&QTcpSocket::disconnected,client,&QObject::deleteLater);
    qInfo() << "node-acq: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; int iParam=0xffff; QIntValidator trigV(256,65535,this);
   qInfo() << "node-acq: <Comm> Received command:" << cmd;
   if (!cmd.contains("=")) {
    sList.append(cmd);
    if (cmd==CMD_ACQ_ACQINFO) {
     qDebug("node-acq: <Comm> Sending Amplifier(s) Info..");
     client->write("-> EEG Samplerate: "+QString::number(conf.eegRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf.refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf.bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(conf.physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(conf.totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(conf.totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
    } else if (cmd==CMD_ACQ_AMPSYNC) {
     qDebug("node-acq: <Comm> Conveying SYNC to amplifier(s)..");
     conf.syncOngoing=true; conf.syncPerformed=false;
     acqThread->sendTrigger(TRIG_AMPSYNC);
     client->write("SYNC conveyed to amps.\n");
    } else if (cmd==CMD_ACQ_STATUS) {
     qDebug("node-acq: <Comm> Sending Amp(s) status..");
     client->write("Amp(s) streaming EEG.\n");
    } else if (cmd==CMD_ACQ_DISCONNECT) { 
     qDebug("node-acq: <Comm> Disconnecting client..");
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
    } else if (cmd==CMD_ACQ_GETCONF) {
     qDebug("node-acq: <Comm> Sending Config Parameters..");
     client->write(QString::number(conf.ampCount).toUtf8()+","+ \
                   QString::number(conf.eegRate).toUtf8()+","+ \
                   QString::number(conf.refChnCount).toUtf8()+","+ \
                   QString::number(conf.bipChnCount).toUtf8()+","+ \
                   QString::number(conf.refGain).toUtf8()+","+ \
                   QString::number(conf.bipGain).toUtf8()+","+ \
                   QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
    } else if (cmd==CMD_ACQ_GETCHAN) {
     qDebug("node-acq: <Comm> Sending Channels' Parameters..");
     for (const auto& ch:conf.chnInfo) {
      QString interMode="";
      for (int idx=0;idx<ch.interMode.size();idx++) interMode.append(QString::number(ch.interMode[idx])+",");
      interMode.remove(interMode.size()-1,1);
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.type).toUtf8()+","+ \
                    interMode.toUtf8()+"\n");
     }
    } else if (cmd==CMD_ACQ_S_TRIG_1000) {
     acqThread->sendTrigger(1000);
    } else if (cmd==CMD_ACQ_S_TRIG_1) {
     acqThread->sendTrigger(1);
    } else if (cmd==CMD_ACQ_S_TRIG_2) {
     acqThread->sendTrigger(2);
    } else if (cmd==CMD_ACQ_DUMPON) {
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
     conf.hEEGStream << quint32(conf.ampCount) << quint32(conf.physChnCount);// << quint32(conf.audioCount);
     conf.dumpRaw=true;
     client->write("node-acq: Raw EEG dumping started.\n");
    } else if (cmd==CMD_ACQ_DUMPOFF) {
     conf.dumpRaw=false; conf.hEEGStream.setDevice(nullptr); conf.hEEGFile.close();
     client->write("node-acq: Raw EEG dumping stopped.\n");
    }
   } else { // command with parameter
    sList=cmd.split("=");
    if (sList[0]=="TRIGGER") {
     int pos=0;
     if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
     if (iParam<0xffff) {
      qDebug("node-acq: <Comm> Conveying **non-hardware** trigger to amplifier(s).. TCode:%d",iParam);
      acqThread->sendTrigger(iParam);
      client->write("**Non-hardware** trigger conveyed to amps.\n");
     } else {
      qDebug() << "node-acq: ERROR!!! <Comm> **Non-hardware** trigger is not between designated interval (256,65535).";
      client->write("Error! **Non-hardware** Trigger should be between (256,65535). Trigger not conveyed.\n");
     }
    } else if (sList[0]==CMD_ACQ_COMPCHAN) {
     QStringList params=sList[1].split(",");
     unsigned int amp=params[0].toInt()-1;
     unsigned int type=params[1].toInt();
     unsigned int chn=params[2].toInt();
     unsigned int state=params[3].toInt();
     {
      QMutexLocker locker(&conf.chnInterMutex);
      for (int chIdx=0;chIdx<conf.chnInfo.size();chIdx++) {
       if (conf.chnInfo[chIdx].type==type && conf.chnInfo[chIdx].physChn==chn) {
        conf.chnInfo[chIdx].interMode[amp]=state;
       }
      }
     }
     //conf.generateElecLists();
     //qDebug() << "node-acq: <Comm> Channel to be computed.. amp:" << amp+1 << "ctype:" << type << "chn:" << chn << "inter:" << state;
     QString response;
     for (int chIdx=0;chIdx<conf.chnInfo.size();chIdx++) {
      response+=QString::number(conf.chnInfo[chIdx].interMode[amp])+",";
     }
     response.remove(response.size()-1,1);
     client->write(response.toUtf8());
     //client->write("Channel to be computed.\n");
    } else {
     qDebug("node-acq: <Comm> Unknown command received..");
     client->write("Unknown command..\n");
    }
   }
  }

  void onNewStrmClient() {
   while (strmServer.hasPendingConnections()) {
    QTcpSocket *client=strmServer.nextPendingConnection();
    
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);   // TCP_NODELAY
    client->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    // optional: smaller buffers to avoid deep OS queues
    client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 64*1024);
    
    strmClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     //for (int i=strmClients.size()-1;i>=0;--i) { QTcpSocket *client=strmClients.at(i);
     // if (client->state()!=QAbstractSocket::ConnectedState) { strmClients.removeAt(i); client->deleteLater(); }
     //}
     qDebug() << "node-acq: <Strm> client from" << client->peerAddress().toString() << "disconnected.";
     strmClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "node-acq: <Strm> client connected from" << client->peerAddress().toString();
   }
  }

  void sendPacket(const QByteArray &packet){
   for (QTcpSocket *client:strmClients) {
    if (!client) continue;
    if (client->state()!=QAbstractSocket::ConnectedState) continue;
    client->write(packet);
    //client->flush(); // write actual serialized block
   }
  }

 private:
  QTcpServer commServer,strmServer;
  QVector<QTcpSocket*> strmClients;
  AcqThread *acqThread; TcpThread *tcpThread;
};
