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
#include "../ampinfo.h"
#include "../sample.h"
#include "../tcpsample.h"
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
   connect(&dataServer,&QTcpServer::newConnection,this,&HAcqDaemon::onNewDataClient);
  }

  bool initialize() {
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf,&chnInfo)) {
#ifdef HACQ_VERBOSE
     qInfo() << "octopus_hacqd: Detailed channels info:";
     qInfo() << "--------------------------------------";
     for (int i=0;i<chnInfo.size();i++)
      qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] Bipolar=%d",chnInfo[i].physChn,qUtf8Printable(chnInfo[i].chnName), \
            chnInfo[i].topoTheta,chnInfo[i].topoPhi,chnInfo[i].topoX,chnInfo[i].topoY,chnInfo[i].isBipolar);
#endif
     ampInfo.ampCount=conf.ampCount;
     ampInfo.sampleRate=conf.sampleRate;
     ampInfo.tcpBufSize=conf.tcpBufSize; // in seconds
     ampInfo.refChnCount=conf.refChnCount; ampInfo.refChnMaxCount=REF_CHN_MAXCOUNT;
     ampInfo.bipChnCount=conf.bipChnCount; ampInfo.bipChnMaxCount=BIP_CHN_MAXCOUNT;
     ampInfo.physChnCount=conf.refChnCount+conf.bipChnCount;
     ampInfo.totalChnCount=ampInfo.physChnCount+2;
     ampInfo.totalCount=conf.ampCount*ampInfo.totalChnCount;
     ampInfo.refGain=conf.refGain;
     ampInfo.bipGain=conf.bipGain;
     ampInfo.eegProbeMsecs=conf.eegProbeMsecs;

     qInfo() << "---------------------------------------------------------------";
     qInfo() << "octopus_hacqd: ---> Datahandling Info <---";
     qInfo() << "octopus_hacqd: Connected # of amplifier(s):" << ampInfo.ampCount;
     qInfo() << "octopus_hacqd: Sample Rate:" << ampInfo.sampleRate;
     qInfo() << "octopus_hacqd: TCP Ringbuffer allocated for" << ampInfo.tcpBufSize << "seconds.";
     qInfo() << "octopus_hacqd: EEG data fetched every" << ampInfo.eegProbeMsecs << "ms.";
     qInfo("octopus_hacqd: Per-amp Physical Channel#: %d (%d+%d)",ampInfo.physChnCount, \
           ampInfo.refChnCount,ampInfo.bipChnCount);
     qInfo() << "octopus_hacqd: Per-amp Total Channel# (with Trig and Offset):" << ampInfo.totalChnCount;
     qInfo() << "octopus_hacqd: Total Channel# from all amps:" << ampInfo.totalCount;
     qInfo() << "octopus_hacqd: Referential channels gain:" << ampInfo.refGain;
     qInfo() << "octopus_hacqd: Bipolar channels gain:" << ampInfo.bipGain;
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "octopus_hacqd: <ServerIP> is" << conf.ipAddr;
 
     if (!commServer.listen(QHostAddress::Any,conf.commPort)) {
      qCritical() << "octopus_hacqd: Cannot start TCP server on <Comm> port:" << conf.commPort;
      return true;
     }
     qInfo() << "octopus_hacqd: <Comm> listening on port" << conf.commPort;

     if (!dataServer.listen(QHostAddress::Any,conf.dataPort)) {
      qCritical() << "octopus_hacqd: Cannot start TCP server on <Data> port:" << conf.dataPort;
      return true;
     }
     qInfo() << "octopus_hacqd: <Data> listening on port" << conf.dataPort;

     hAcqThread=new HAcqThread(&ampInfo,ampInfo.tcpBufSize*ampInfo.sampleRate,&tcpMutex,this);
     connect(hAcqThread,&HAcqThread::sendData,this,&HAcqDaemon::drainAndBroadcast);
     hAcqThread->start();

     return false;
    }
    qWarning() << "octopus_hacqd: The config file" << cfgPath << "is corrupt!";
    return true;
   }
   qWarning() << "octopus_hacqd: The config file" << cfgPath << "does not exist!";
   return true;
  }

  AmpInfo ampInfo;
  QMutex tcpMutex;

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
    if (cmd==CMD_ACQINFO_S) iCmd=CMD_ACQINFO;
    else if (cmd==CMD_AMPSYNC_S) iCmd=CMD_AMPSYNC;
    else if (cmd==CMD_STATUS_S) iCmd=CMD_STATUS;
    else if (cmd==CMD_DISCONNECT_S) iCmd=CMD_DISCONNECT;
    else if (cmd==CMD_GETCONF_S) iCmd=CMD_GETCONF;
    else if (cmd==CMD_GETCHAN_S) iCmd=CMD_GETCHAN;
    else if (cmd==CMD_REBOOT_S) iCmd=CMD_REBOOT;
    else if (cmd==CMD_SHUTDOWN_S) iCmd=CMD_SHUTDOWN;
   } else { // command with parameter
    sList=cmd.split("|");
    if (sList[0]=="TRIGGER") {
     iCmd=CMD_TRIGGER; int pos=0;
     if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
    }
   }
   switch (iCmd) {
    case CMD_ACQINFO: qDebug("octopus_hacqd: <Comm> Sending Amplifier(s) Info..");
     client->write("-> Samplerate: "+QString::number(ampInfo.sampleRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(ampInfo.refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(ampInfo.bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(ampInfo.physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(ampInfo.totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(ampInfo.totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(ampInfo.eegProbeMsecs).toUtf8()+"\n");
     break;
    case CMD_TRIGGER:
     if (iParam<0xffff) {
      qDebug("octopus_hacqd: <Comm> Conveying **non-hardware** trigger to amplifier(s).. TCode:%d",iParam);
      hAcqThread->sendTrigger(iParam);
      client->write("**Non-hardware** trigger conveyed to amps.\n");
     } else {
      qDebug() << "octopus_hacqd: ERROR!!! <Comm> **Non-hardware** trigger is not between designated interval (256,65535).";
      client->write("Error! **Non-hardware** Trigger should be between (256,65535). Trigger not conveyed.\n");
     }
     break;
    case CMD_AMPSYNC: qDebug("octopus_hacqd: <Comm> Conveying SYNC to amplifier(s)..");
     hAcqThread->sendTrigger(TRIG_AMPSYNC);
     client->write("SYNC conveyed to amps.\n");
     break;
    case CMD_STATUS: qDebug("octopus_hacqd: <Comm> Sending Amp(s) status..");
     client->write("Amp(s) streaming EEG.\n");
     break;
    case CMD_DISCONNECT: qDebug("octopus_hacqd: <Comm> Disconnecting client..");
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
     break;
    case CMD_GETCONF: qDebug("octopus_hacqd: <Comm> Sending Config Parameters..");
     client->write(QString::number(ampInfo.ampCount).toUtf8()+","+ \
                   QString::number(ampInfo.sampleRate).toUtf8()+","+ \
                   QString::number(ampInfo.refChnCount).toUtf8()+","+ \
                   QString::number(ampInfo.bipChnCount).toUtf8()+","+ \
                   QString::number(ampInfo.refGain).toUtf8()+","+ \
                   QString::number(ampInfo.bipGain).toUtf8()+","+ \
                   QString::number(ampInfo.eegProbeMsecs).toUtf8()+"\n");
     break;
    case CMD_GETCHAN: qDebug("octopus_hacqd: <Comm> Sending Channels' Parameters..");
     for (int i=0;i<chnInfo.size();i++)
      client->write(QString::number(chnInfo[i].physChn).toUtf8()+","+ \
                    QString(chnInfo[i].chnName).toUtf8()+","+ \
                    QString::number(chnInfo[i].topoTheta).toUtf8()+","+ \
                    QString::number(chnInfo[i].topoPhi).toUtf8()+","+ \
                    QString::number(chnInfo[i].isBipolar).toUtf8()+"\n");
     break;
    case CMD_REBOOT: qDebug("octopus_hacqd: <Comm> Rebooting server (if privileges are enough)..");
     client->write("Rebooting system (if privileges are enough)...\n");
     system("/sbin/shutdown -r now");
     break;
    case CMD_SHUTDOWN: qDebug("octopus_hacqd: <Comm> Shutting down server (if privileges are enough)..");
     client->write("Shutting down system (if privileges are enough)...\n");
     system("/sbin/shutdown -h now");
     break;
    default: qDebug("octopus_hacqd: <Comm> Unknown command received..");
     client->write("Unknown command..\n");
     break;
   }
  }

  void onNewDataClient() {
   while (dataServer.hasPendingConnections()) {
    QTcpSocket *client=dataServer.nextPendingConnection();
    dataClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     //for (int i=dataClients.size()-1;i>=0;--i) {
     // QTcpSocket *client=dataClients.at(i);
     // if (client->state()!=QAbstractSocket::ConnectedState) {
     //  dataClients.removeAt(i);
     //  client->deleteLater();
     // }
     //}
     qDebug() << "octopus_hacqd: <Data> client from" << client->peerAddress().toString() << "disconnected.";
     dataClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "octopus_hacqd: <Data> client connected from" << client->peerAddress().toString();
   }
  }

  void drainAndBroadcast() {
   TcpSample tcpSample(ampInfo.ampCount,ampInfo.physChnCount);
   while (hAcqThread->popSample(&tcpSample)) {
    QByteArray payLoad=tcpSample.serialize();
    
    //qDebug() << "Daemon: ampCount:" << tcpSample.amp.size() 
    //     << " ChCount:" << tcpSample.amp[0].dataF.size() 
    //     << " FirstVal:" << tcpSample.amp[0].dataF[0];
         
    for (QTcpSocket *client : dataClients) {
     if (client->state()==QAbstractSocket::ConnectedState) {
      QDataStream sizeStream(client); sizeStream.setByteOrder(QDataStream::LittleEndian);
      quint32 msgLength=static_cast<quint32>(payLoad.size()); // write message length first
      sizeStream<<msgLength;
      client->write(payLoad);
      client->flush(); // write actual serialized block
     }
    }
   }
  }

/* loopback deserialization test
  void drainAndBroadcast() {
   TcpSample tcpSample(ampInfo.ampCount,ampInfo.physChnCount);
   while (hAcqThread->popSample(&tcpSample)) {
    QByteArray payLoad = tcpSample.serialize();
    // Loopback Test — Deserialize immediately to verify what we are about to send
    QDataStream in(&payLoad, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 trigger = 0, ampCount = 0;
    in >> trigger >> ampCount;


    for (quint32 i = 0; i < ampCount; ++i) {
     if (i==0) qDebug() << "[Loopback] trigger =" << trigger << "ampCount =" << ampCount;
     float marker = 0;
     in >> marker;
     if (i==0) qDebug() << "[Loopback] amp =" << i << "marker =" << marker;

     auto readFloatVector = [&](int size, const char* label) {
        QString line;
        for (int k = 0; k < size; ++k) {
            float val;
            in >> val;
            if (k < 3)  // Only print first few for sanity check
                line += QString("%1 %2: %3  ").arg(label).arg(k).arg(val, 0, 'g', 6);
        }
        qDebug() << line;
     };
     readFloatVector(tcpSample.amp[i].data.size(), "raw");
     readFloatVector(tcpSample.amp[i].dataF.size(), "flt");
    }
    qDebug() << "[Loopback] payload size =" << payLoad.size() << " bytes read =" << in.device()->pos();
   }
  }
*/

 private:
  ConfParam conf;
  QVector<ChnInfo> chnInfo;
  QTcpServer commServer,dataServer;
  QVector<QTcpSocket*> dataClients;
  HAcqThread *hAcqThread;
};

#endif
