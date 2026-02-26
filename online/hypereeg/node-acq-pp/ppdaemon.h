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
#include <QIntValidator>
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
#include "compthread.h"
#include "tcpthread.h"

class PPDaemon: public QObject {
 Q_OBJECT
 public:
  explicit PPDaemon(QObject *parent=nullptr,ConfParam *c=nullptr) : QObject(parent) { conf=c; }

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

   conf->frameBytesOut=TcpSamplePP(conf->ampCount,conf->physChnCount).serialize().size();

   // CHANNELS

   commResponse=conf->commandToDaemon(conf->acqCommSocket,CMD_ACQ_GETCHAN);
   if (!commResponse.isEmpty()) qInfo() << "<GetChannelListFromAcqDaemon> ChannelList received."; // << commResponse;
   else qCritical() << "<GetChannelListFromAcqDaemon> (TIMEOUT) No response from Acquisition Node!";
   sList=commResponse.split("\n"); PPChnInfo chn;

   chn.interMode.resize(conf->ampCount);
   for (int chnIdx=0;chnIdx<sList.size();chnIdx++) { // Individual CHANNELs information
    sList2=sList[chnIdx].split(",");
    chn.physChn=sList2[0].toInt();
    chn.chnName=sList2[1];
    chn.topoTheta=sList2[2].toFloat();
    chn.topoPhi=sList2[3].toFloat();
    chn.topoX=sList2[4].toInt();
    chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    for (unsigned int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) chn.interMode[ampIdx]=sList2[7+ampIdx].toInt();
    conf->chnInfo.append(chn);
   }
   conf->chnCount=conf->chnInfo.size();

   qDebug() << "[PP] ampCount=" << conf->ampCount
            << "chnCount=" << conf->chnCount
            << "physChnCount=" << conf->physChnCount
            << "frameBytesIn=" << conf->frameBytesIn;

   // Constants or calculated global settings upon the ones read from config file
   conf->tcpBuffer=QVector<TcpSamplePP>(conf->tcpBufSize,TcpSamplePP(conf->ampCount,conf->chnCount));

   if (!conf->acqppCommServer.listen(QHostAddress::Any,conf->acqppCommPort)) {
    qCritical() << "<Comm> Cannot start TCP server on <Comm> port:" << conf->acqppCommPort;
    return true;
   }
   if (!conf->acqppStrmServer.listen(QHostAddress::Any,conf->acqppStrmPort)) {
    qCritical() << "<Comm> Cannot start TCP server on <Strm> port:" << conf->acqppStrmPort;
    return true;
   }

   //for (int idx=0;idx<conf->chnInfo.size();idx++) {
   // QString x="";
   // for (int j=0;j<conf->ampCount;j++) x.append(QString::number(conf->chnInfo[idx].interMode[j])+",");
   // qCritical() << x.toUtf8();
   //}

   conf->initFilters();

   // We're server
   connect(&(conf->acqppCommServer),&QTcpServer::newConnection,this,&PPDaemon::onNewCommClient);
   connect(&(conf->acqppStrmServer),&QTcpServer::newConnection,this,&PPDaemon::onNewStrmClient);

   // Setup data socket -- only safe after handshake and receiving crucial info about streaming
   connect(conf->acqStrmSocket,&QTcpSocket::readyRead,conf,&ConfParam::onStrmDataReady); // TCP handler for instream

   // Connect for streaming data -- only safe after handshake and receiving crucial info about streaming
   conf->acqStrmSocket->connectToHost(conf->acqIpAddr,conf->acqStrmPort); conf->acqStrmSocket->waitForConnected();

   // Threads
   compThread=new CompThread(conf,this);
   compThread->start(QThread::HighestPriority);

   tcpThread=new TcpThread(conf,this);
   connect(tcpThread,&TcpThread::sendPacketSignal,this,&PPDaemon::sendPacket,Qt::QueuedConnection); // Send packets timely
   tcpThread->start(QThread::HighestPriority);

   return false;
  }

//conf->compStop.store(true);
//if (compThread) {
//  compThread->requestInterruption();
//  conf->compMutex.lock();
//  conf->compReady.wakeAll();
//  conf->compMutex.unlock();
//  compThread->wait();
//}

  ConfParam *conf;

 private slots:
  void onNewCommClient() {
   while (conf->acqppCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->acqppCommServer.nextPendingConnection();
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
   QStringList sList; QIntValidator trigV(256,65535,this);
   qInfo() << "<Comm> Received command:" << cmd;
   if (!cmd.contains("=")) {
    sList.append(cmd);
    if (cmd==CMD_ACQ_ACQINFO) {
     qInfo() << "<Comm> Sending Amplifier(s) Info..";
     client->write("-> EEG Samplerate: "+QString::number(conf->eegRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf->refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf->bipChnCount).toUtf8()+"\n");
     client->write("-> Physical channel(s)# (Ref+Bip): "+QString::number(conf->physChnCount).toUtf8()+"\n");
     client->write("-> Total channels# (Ref+Bip+Trig+Offset): "+QString::number(conf->totalChnCount).toUtf8()+"\n");
     client->write("-> Grand total channels# from all amps: "+QString::number(conf->totalCount).toUtf8()+"\n");
     client->write("-> EEG Probe interval (ms): "+QString::number(conf->eegProbeMsecs).toUtf8()+"\n");
    } else if (cmd==CMD_ACQ_GETCONF) {
     qInfo() << "<Comm> Sending Config Parameters..";
     client->write(QString::number(conf->ampCount).toUtf8()+","+ \
                   QString::number(conf->eegRate).toUtf8()+","+ \
                   QString::number(conf->refChnCount).toUtf8()+","+ \
                   QString::number(conf->bipChnCount).toUtf8()+","+ \
                   QString::number(conf->physChnCount).toUtf8()+","+ \
                   QString::number(conf->totalChnCount).toUtf8()+","+ \
                   QString::number(conf->totalCount).toUtf8()+","+ \
                   QString::number(conf->refGain).toUtf8()+","+ \
                   QString::number(conf->bipGain).toUtf8()+","+ \
                   QString::number(conf->eegProbeMsecs).toUtf8()+","+ \
                   QString::number(conf->frameBytesOut).toUtf8()+"\n");
    } else if (cmd==CMD_ACQ_GETCHAN) {
     qInfo() << "<Comm> Sending Channels' Parameters..";
     for (const auto& ch:conf->chnInfo) {
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
    } else if (cmd==CMD_ACQ_STATUS) {
     qInfo() << "<Comm> Sending Amp(s) status..";
     client->write("Amp(s) streaming EEG.\n");
    } else if (cmd==CMD_ACQ_DISCONNECT) { 
     qInfo() << "<Comm> Disconnecting client..";
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
    }
   } else { // command with parameter
    sList=cmd.split("=");
    if (sList[0]==CMD_ACQ_COMPCHAN) {
     QStringList params=sList[1].split(",");
     unsigned int amp=params[0].toInt()-1;
     unsigned int type=params[1].toInt();
     unsigned int chn=params[2].toInt();
     unsigned int state=params[3].toInt();
     {
      QMutexLocker locker(&(conf->chnInterMutex));
      for (int chIdx=0;chIdx<conf->chnInfo.size();chIdx++) {
       if (conf->chnInfo[chIdx].type==type && conf->chnInfo[chIdx].physChn==chn) {
        conf->chnInfo[chIdx].interMode[amp]=state;
       }
      }
     }
     //conf->generateElecLists();
     //qInfo() << "<Comm> Channel to be computed.. amp:" << amp+1 << "ctype:" << type << "chn:" << chn << "inter:" << state;
     QString response;
     for (int chIdx=0;chIdx<conf->chnInfo.size();chIdx++) {
      response+=QString::number(conf->chnInfo[chIdx].interMode[amp])+",";
     }
     response.remove(response.size()-1,1);
     client->write(response.toUtf8());
     //client->write("Channel to be computed.\n");
    } else {
     qWarning() << "<Comm> Unknown command received..";
     client->write("node-acq: Unknown command..\n");
    }
   }
  }

  void onNewStrmClient() {
   while (conf->acqppStrmServer.hasPendingConnections()) {
    QTcpSocket *client=conf->acqppStrmServer.nextPendingConnection();
    
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);   // TCP_NODELAY
    client->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    // optional: smaller buffers to avoid deep OS queues
    client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 64*1024);
    
    strmClients.append(client);
    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     //for (int i=strmClients.size()-1;i>=0;--i) { QTcpSocket *client=strmClients.at(i);
     // if (client->state()!=QAbstractSocket::ConnectedState) { strmClients.removeAt(i); client->deleteLater(); }
     //}
     qInfo() << "<Strm> client from" << client->peerAddress().toString() << "disconnected.";
     strmClients.removeAll(client);
     client->deleteLater();
    });
    qInfo() << "<Strm> client connected from" << client->peerAddress().toString();
   }
  }

  void sendPacket(const QByteArray &packet){
   for (QTcpSocket *client:strmClients) {
    if (!client) continue;
    if (client->state()!=QAbstractSocket::ConnectedState) continue;
    client->write(packet);
    //client->flush(); // write actual serialized block

    static qint64 lastTx=0;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastTx >= 1000) {
     lastTx = now;
     qInfo() << "[PP:TX] bytesToWrite=" << client->bytesToWrite();
    }
   }
  }

 private:
  QVector<QTcpSocket*> strmClients;
  CompThread *compThread; TcpThread *tcpThread;
};
