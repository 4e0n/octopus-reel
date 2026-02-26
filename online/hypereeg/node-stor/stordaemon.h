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
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
//#include "confparam.h"
#include "recthread.h"

const QString RECROOTDIR="/opt/octopus/stor/heeg";

class StorDaemon: public QObject {
 Q_OBJECT
 public:
  explicit StorDaemon(QObject *parent=nullptr,ConfParam *c=nullptr) : QObject(parent) { conf=c; }

  bool start() { QString commResponse; QStringList sList,sList2;

   // We're client of node-acq
   conf->acqCommSocket=new QTcpSocket(this);
   conf->acqCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf->acqCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   conf->acqStrmSocket=new QTcpSocket(this);
   conf->acqStrmSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf->acqStrmSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);

   conf->acqCommSocket->connectToHost(conf->acqIpAddr,conf->acqCommPort); conf->acqCommSocket->waitForConnected();


   // Get crucial info from the "master" node we connect to
   commResponse=conf->commandToDaemon(conf->acqCommSocket,CMD_ACQ_GETCONF);
   if (!commResponse.isEmpty()) qInfo() << "<GetConfFromAcqDaemon> Reply:" << commResponse;
   else qCritical() << "<GetConfFromAcqDaemon> (TIMEOUT) No response from Acquisition Node!";
   sList=commResponse.split(",");
   conf->ampCount=sList[0].toInt(); // (ACTUAL) AMPCOUNT
   conf->eegRate=sList[1].toInt();  // EEG SAMPLERATE
   conf->tcpBufSize*=conf->eegRate;          // TCPBUFSIZE (in SAMPLE#)
   conf->refChnCount=sList[2].toInt();
   conf->bipChnCount=sList[3].toInt();
   conf->physChnCount=sList[4].toInt();
   conf->totalChnCount=sList[5].toInt();
   conf->totalCount=sList[6].toInt();
   conf->refGain=sList[7].toFloat();
   conf->bipGain=sList[8].toFloat();
   conf->eegProbeMsecs=sList[9].toInt(); // This determines the (maximum/optimal) data feed rate together with eegRate
   conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;
   conf->frameBytesIn=sList[10].toInt();

   // CHANNELS
   commResponse=conf->commandToDaemon(conf->acqCommSocket,CMD_ACQ_GETCHAN);
   if (!commResponse.isEmpty()) qInfo() << "<GetChannelListFromAcqDaemon> ChannelList received."; // << commResponse;
   else qCritical() << "<GetChannelListFromAcqDaemon> (TIMEOUT) No response from Acquisition Node!";
   sList=commResponse.split("\n"); StorChnInfo chn;

   for (int chnIdx=0;chnIdx<sList.size();chnIdx++) { // Individual CHANNELs information
    sList2=sList[chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.type=(bool)sList2[6].toInt();
    conf->chnInfo.append(chn);
   }
   conf->chnCount=conf->chnInfo.size();

   // Constants or calculated global settings upon the ones read from config file
   conf->tcpBuffer=QVector<TcpSample>(conf->tcpBufSize,TcpSample(conf->ampCount,conf->chnCount));

   if (!conf->storCommServer.listen(QHostAddress::Any,conf->storCommPort)) {
    qCritical() << "<Comm> Cannot start TCP server on <Comm> port:" << conf->storCommPort;
    return true;
   }

   //for (int idx=0;idx<conf->chnInfo.size();idx++) {
   // QString x="";
   // for (int j=0;j<conf->ampCount;j++) x.append(QString::number(conf->chnInfo[idx].interMode[j])+",");
   // qCritical() << x.toUtf8();
   //}

   // We're server
   connect(&(conf->storCommServer),&QTcpServer::newConnection,this,&StorDaemon::onNewCommClient);

   // Setup data socket -- only safe after handshake and receiving crucial info about streaming
   connect(conf->acqStrmSocket,&QTcpSocket::readyRead,conf,&ConfParam::onStrmDataReady); // TCP handler for instream

   // Connect for streaming data -- only safe after handshake and receiving crucial info about streaming
   conf->acqStrmSocket->connectToHost(conf->acqIpAddr,conf->acqStrmPort); conf->acqStrmSocket->waitForConnected();

   // Threads should be here
   recThread=new RecThread(conf,this);
   recThread->start(QThread::HighestPriority);

   return false;
  }

  ConfParam *conf;

 private slots:
  void onNewCommClient() {
   while (conf->storCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->storCommServer.nextPendingConnection();
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
  RecThread *recThread;
};
