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
#include <QProcess>
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

   // We're client of the "origin" node (e.g. node-acq)
   conf->origCommSocket=new QTcpSocket(this);
   conf->origCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf->origCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   conf->origStrmSocket=new QTcpSocket(this);
   conf->origStrmSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf->origStrmSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);

   qDebug() << conf->origIpAddr << conf->origCommPort;
   conf->origCommSocket->connectToHost(conf->origIpAddr,conf->origCommPort); conf->origCommSocket->waitForConnected();

   // Get crucial info from the "master" node we connect to
   commResponse=conf->commandToDaemon(conf->origCommSocket,CMD_ACQ_GETCONF);
   if (!commResponse.isEmpty()) qInfo() << "<GetConfFromAcqDaemon> Reply:" << commResponse;
   else qCritical() << "<GetConfFromAcqDaemon> (TIMEOUT) No response from Acquisition Node!";
   sList=commResponse.split(",");
   conf->ampCount=sList[0].toInt(); // (ACTUAL) AMPCOUNT
   conf->eegRate=sList[1].toInt();  // EEG SAMPLERATE
   conf->tcpBufSize*=conf->eegRate;          // TCPBUFSIZE (in SAMPLE#)
   conf->refChnCount=sList[2].toInt();
   conf->bipChnCount=sList[3].toInt();
   conf->metaChnCount=sList[4].toInt();
   conf->physChnCount=sList[5].toInt();
   conf->chnCount=sList[6].toInt();
   conf->totalChnCount=sList[7].toInt();
   conf->totalCount=sList[8].toInt();
   conf->refGain=sList[9].toFloat();
   conf->bipGain=sList[10].toFloat();
   conf->eegProbeMsecs=sList[11].toInt(); // This determines the (maximum/optimal) data feed rate together with eegRate
   conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;
   conf->frameBytesIn=sList[12].toInt();

   const unsigned int ampCount=conf->ampCount; const unsigned int chnCount=conf->chnCount;
   const unsigned int refChnCount=conf->refChnCount; const unsigned int bipChnCount=conf->bipChnCount; 
   const unsigned int metaChnCount=conf->metaChnCount; const unsigned int physChnCount=conf->physChnCount; 

   conf->frameBytesOut=TcpSamplePP(ampCount,chnCount).serialize().size();

   // CHANNELS
   //
   commResponse=conf->commandToDaemon(conf->origCommSocket,CMD_ACQ_GETCHAN);
   if (!commResponse.isEmpty()) qInfo() << "<GetChannelListFromAcqDaemon> ChannelList received."; // << commResponse;
   else qCritical() << "<GetChannelListFromAcqDaemon> (TIMEOUT) No response from Acquisition Node!";
   sList=commResponse.split("\n");
   
   //qDebug() << "sList.size()=" << sList.size() << "chnCount:" << chnCount;
   for (unsigned int chnIdx=0;chnIdx<refChnCount;chnIdx++) { // Individual CHANNELs information
    PPChnInfo chn; chn.interMode.resize(ampCount);
    sList2=sList[chnIdx].split(",");
    chn.physChn=sList2[0].toInt();
    chn.chnName=sList2[1];
    chn.topoTheta=sList2[2].toFloat();
    chn.topoPhi=sList2[3].toFloat();
    chn.topoX=sList2[4].toInt();
    chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    const int ieCount=sList2[7].toInt();
    // Electrodes all have their value originally, later can be changed via node-time operator input
    for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) chn.interMode[ampIdx]=1;
    chn.interElec.clear();
    for (int ieChnIdx=0;ieChnIdx<ieCount;ieChnIdx++) chn.interElec.append(sList2[8+ieChnIdx].toInt());
    conf->refChns.append(chn);
   }
   for (unsigned int chnIdx=0;chnIdx<bipChnCount;chnIdx++) { // Individual CHANNELs information
    PPChnInfo chn; chn.interMode.resize(ampCount);
    sList2=sList[refChnCount+chnIdx].split(",");
    chn.physChn=sList2[0].toInt();
    chn.chnName=sList2[1];
    chn.topoTheta=sList2[2].toFloat();
    chn.topoPhi=sList2[3].toFloat();
    chn.topoX=sList2[4].toInt();
    chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    const int ieCount=sList2[7].toInt();
    // Electrodes all have their value originally, later can be changed via node-time operator input
    for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) chn.interMode[ampIdx]=0;
    chn.interElec.clear();
    for (int ieChnIdx=0;ieChnIdx<ieCount;ieChnIdx++) chn.interElec.append(sList2[8+ieChnIdx].toInt());
    conf->bipChns.append(chn);
   }
   for (unsigned int chnIdx=0;chnIdx<metaChnCount;chnIdx++) { // Individual CHANNELs information
    PPChnInfo chn; chn.interMode.resize(ampCount);
    sList2=sList[physChnCount+chnIdx].split(",");
    chn.physChn=sList2[0].toInt();
    chn.chnName=sList2[1];
    chn.topoTheta=sList2[2].toFloat();
    chn.topoPhi=sList2[3].toFloat();
    chn.topoX=sList2[4].toInt();
    chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    const int ieCount=sList2[7].toInt();
    // Electrodes all have their value originally, later can be changed via node-time operator input
    for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) chn.interMode[ampIdx]=0;
    chn.interElec.clear();
    for (int ieChnIdx=0;ieChnIdx<ieCount;ieChnIdx++) chn.interElec.append(sList2[8+ieChnIdx].toInt());
    conf->metaChns.append(chn);
   }

   qDebug() << "[PP] ampCount=" << ampCount << "chnCount=" << chnCount
            << "physChnCount=" << conf->physChnCount << "frameBytesIn=" << conf->frameBytesIn;

   // Constants or calculated global settings upon the ones read from config file
   conf->tcpBuffer=QVector<TcpSamplePP>(conf->tcpBufSize,TcpSamplePP(ampCount,chnCount));

   if (!conf->compCommServer.listen(QHostAddress::Any,conf->compCommPort)) {
    qCritical() << "<Comm> Cannot start TCP server on <Comm> port:" << conf->compCommPort;
    return true;
   }
   if (!conf->compStrmServer.listen(QHostAddress::Any,conf->compStrmPort)) {
    qCritical() << "<Comm> Cannot start TCP server on <Strm> port:" << conf->compStrmPort;
    return true;
   }

   //for (int idx=0;idx<conf->refChns.size();idx++) {
   // QString x="";
   // for (int j=0;j<ampCount;j++) x.append(QString::number(conf->refChns[idx].interMode[j])+",");
   // qCritical() << x.toUtf8();
   //}

   conf->initialize();

   // We're server
   connect(&(conf->compCommServer),&QTcpServer::newConnection,this,&PPDaemon::onNewCommClient);
   connect(&(conf->compStrmServer),&QTcpServer::newConnection,this,&PPDaemon::onNewStrmClient);

   // Setup data socket -- only safe after handshake and receiving crucial info about streaming
   connect(conf->origStrmSocket,&QTcpSocket::readyRead,conf,&ConfParam::onStrmDataReady); // TCP handler for instream

   // Connect for streaming data -- only safe after handshake and receiving crucial info about streaming
   conf->origStrmSocket->connectToHost(conf->origIpAddr,conf->origStrmPort); conf->origStrmSocket->waitForConnected();

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
   while (conf->compCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->compCommServer.nextPendingConnection();
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
   //qInfo() << "<Comm> Received command:" << cmd;
   if (!cmd.contains("=")) {
    sList.append(cmd);
    if (cmd==CMD_ACQ_ACQINFO) {
     qInfo() << "<Comm> Sending Amplifier(s) Info..";
     client->write("-> EEG Samplerate: "+QString::number(conf->eegRate).toUtf8()+"sps\n");
     client->write("-> Referential channel(s)#: "+QString::number(conf->refChnCount).toUtf8()+"\n");
     client->write("-> Bipolar channel(s)#: "+QString::number(conf->bipChnCount).toUtf8()+"\n");
     client->write("-> Meta (computed) channel(s)#: "+QString::number(conf->metaChnCount).toUtf8()+"\n");
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
                   QString::number(conf->metaChnCount).toUtf8()+","+ \
                   QString::number(conf->physChnCount).toUtf8()+","+ \
                   QString::number(conf->chnCount).toUtf8()+","+ \
                   QString::number(conf->totalChnCount).toUtf8()+","+ \
                   QString::number(conf->totalCount).toUtf8()+","+ \
                   QString::number(conf->refGain).toUtf8()+","+ \
                   QString::number(conf->bipGain).toUtf8()+","+ \
                   QString::number(conf->eegProbeMsecs).toUtf8()+","+ \
                   QString::number(conf->frameBytesOut).toUtf8()+"\n");
    } else if (cmd==CMD_ACQ_GETCHAN) {
     qInfo() << "<Comm> Sending Channels' Parameters..";
     for (const auto& ch:conf->refChns) {
      QString interMode="";
      for (int idx=0;idx<ch.interMode.size();idx++) interMode.append(QString::number(ch.interMode[idx])+",");
      if (!interMode.isEmpty()) interMode.chop(1);
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.type).toUtf8()+","+ \
                    interMode.toUtf8()+"\n"); // interMode status of that channel for individual amps
     }
     for (const auto& ch:conf->bipChns) {
      QString interMode="";
      for (int idx=0;idx<ch.interMode.size();idx++) interMode.append(QString::number(ch.interMode[idx])+",");
      if (!interMode.isEmpty()) interMode.chop(1);
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.type).toUtf8()+","+ \
                    interMode.toUtf8()+"\n"); // interMode status of that channel for individual amps
     }
     for (const auto& ch:conf->metaChns) {
      QString interMode="";
      for (int idx=0;idx<ch.interMode.size();idx++) interMode.append(QString::number(ch.interMode[idx])+",");
      if (!interMode.isEmpty()) interMode.chop(1);
      client->write(QString::number(ch.physChn).toUtf8()+","+ \
                    QString(ch.chnName).toUtf8()+","+ \
                    QString::number(ch.topoTheta).toUtf8()+","+ \
                    QString::number(ch.topoPhi).toUtf8()+","+ \
                    QString::number(ch.topoX).toUtf8()+","+ \
                    QString::number(ch.topoY).toUtf8()+","+ \
                    QString::number(ch.type).toUtf8()+","+ \
                    interMode.toUtf8()+"\n"); // interMode status of that channel for individual amps
     }
    } else if (cmd==CMD_ACQPP_GETCMLEVELS) {
     QStringList vals;
     {
       QMutexLocker lk(&conf->cmMutex);
       if (!conf->cmLevelsValid) { client->write("\n"); client->flush(); return; }
       for (unsigned int ampIdx=0;ampIdx<conf->ampCount;++ampIdx) {
       if (ampIdx>=unsigned(conf->latestCMLevels.size())) break;
       for (unsigned int chnIdx=0;chnIdx<conf->chnCount;++chnIdx) {
        if ((int)chnIdx>=conf->latestCMLevels[(int)ampIdx].size()) break;
        vals << QString::number(conf->latestCMLevels[int(ampIdx)][chnIdx],'f',6);
       }
      }
     }
     client->write(vals.join(",").toUtf8() + "\n");
     client->flush();
    } else if (cmd==CMD_STATUS) {
     //qInfo() << "<Comm> Sending Amp(s) status..";
     client->write("OK\n");
    } else if (cmd==CMD_DISCONNECT) { 
     qInfo() << "<Comm> Disconnecting client..";
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
    } else if (cmd==CMD_REBOOT) {
     const int delaySec=5;
     const QString cmd=QString("sleep %1; /usr/bin/systemctl reboot -i").arg(delaySec);
     QProcess::startDetached("/bin/sh", {"-c",cmd});
     client->disconnectFromHost();
    } else if (cmd==CMD_SHUTDOWN) {
     const int delaySec=5;
     const QString cmd=QString("sleep %1; /usr/bin/systemctl poweroff -i").arg(delaySec);
     QProcess::startDetached("/bin/sh", {"-c",cmd});
     client->disconnectFromHost();
    }
   } else { // command with parameter
    sList=cmd.split("=");
    if (sList[0]==CMD_ACQ_COMPCHAN) { // Handle online operator demands of electrode interpolation/disabling related changes
     QStringList params=sList[1].split(",");
     if (params.size()!=4) { client->write("ERR: invalid params\n"); return; }
     const unsigned int amp=params[0].toInt()-1;
     if (amp >= conf->ampCount) { client->write("ERR: invalid amp index\n"); return; }
     const unsigned int type=params[1].toInt();
     const unsigned int chn=params[2].toInt();
     const unsigned int state=params[3].toInt();
     QString response;
     {
       QMutexLocker locker(&(conf->chnInterMutex));
       QVector<PPChnInfo>* chns=nullptr;
       switch (type) {
        case 0: chns=&conf->refChns; break;
        case 1: chns=&conf->bipChns; break;
        case 2: chns=&conf->metaChns; break; // This doesn't make much sense but let it stay for now
        default: client->write("ERR: invalid channel type\n"); return;
       }
       if (chns) {
        for (int chnIdx=0;chnIdx<chns->size();++chnIdx) {
         if ((*chns)[chnIdx].physChn==chn) { (*chns)[chnIdx].interMode[amp]=state; break; }
        }
        for (int chnIdx=0;chnIdx<chns->size();++chnIdx) response+=QString::number((*chns)[chnIdx].interMode[amp])+",";
       }
     }
     qInfo() << "<Comm> Channel to be computed.. amp:" << amp+1 << "ctype:" << type << "chn:" << chn << "inter:" << state;
     if (!response.isEmpty()) response.chop(1);
     client->write(response.toUtf8());
    } else {
     qWarning() << "<Comm> Unknown command received..";
     client->write("node-comp: Unknown command..\n");
    }
   }
  }

  void onNewStrmClient() {
   while (conf->compStrmServer.hasPendingConnections()) {
    QTcpSocket *client=conf->compStrmServer.nextPendingConnection();
    
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
#ifdef PLL_VERBOSE
     qInfo() << "[PP:TX] bytesToWrite=" << client->bytesToWrite();
#endif
    }
   }
  }

 private:
  QString buildCMLevelsReply(unsigned int ampCount,unsigned int chnCount,quint64 pollCounter) {
   QStringList vals;
   for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
    for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
     int v=0;
     // Alternating ascending / descending debug pattern
     if ((pollCounter%2)==0) v=(int(ampIdx)*40+int(chnIdx)*8)%256;
     else v=(255-((int(ampIdx)*40+int(chnIdx)*8)%256));
     vals << QString::number(v);
    }
   }
   return vals.join(",")+"\n";
  }
  quint64 cmPollCounter=0;

  QVector<QTcpSocket*> strmClients;
  CompThread *compThread; TcpThread *tcpThread;
};
