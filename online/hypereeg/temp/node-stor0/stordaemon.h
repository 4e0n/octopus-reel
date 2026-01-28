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

/* a.k.a. The EEG "Storage Daemon".. */

#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QVector>
#include <QIntValidator>
#include <QDir>
#include "../common/globals.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
//#include "chninfo.h"
#include "recthread.h"

const QString cfgPathX=confPath+"hypereeg.conf";
class StorDaemon: public QObject {
 Q_OBJECT
 public:
  explicit StorDaemon(QObject *parent=nullptr) : QObject(parent) {
   // Downstream (we're client)
   conf.commSocket=new QTcpSocket(this);
   conf.commSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.commSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   conf.strmSocket=new QTcpSocket(this);
   conf.strmSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.strmSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // Upstream (we're server)
   connect(&commServer,&QTcpServer::newConnection,this,&StorDaemon::onNewCommClient);
  }

  bool initialize() { QString cfgPath; //QString commResponse; QStringList sList,sList2;
   if (cfgPathX=="~") cfgPath=QDir::homePath();
   if (cfgPathX.startsWith("~/")) cfgPath=QDir::homePath()+cfgPathX.mid(1);
   if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "node_stor: <ServerIP> is" << conf.ipAddr;
     qInfo() << "node_stor: <Comm> Server ports are (comm,strm):" << conf.commPort << conf.strmPort;

     //conf.tcpEEGBufSize=conf.tcpBufSize*conf.eegRate;
     //conf.tcpEEGBuffer=QVector<TcpSample>(conf.tcpEEGBufSize,TcpSample(conf.ampCount,conf.physChnCount));

     // Setup data socket -- only safe after handshake and receiving crucial info about streaming
     connect(conf.strmSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onStrmDataReady);
     conf.strmSocket->connectToHost(conf.ipAddr,conf.strmPort); conf.strmSocket->waitForConnected();

     if (!commServer.listen(QHostAddress::Any,conf.svrCommPort)) {
      qCritical() << "node_stor: Cannot start TCP server on <Comm> port:" << conf.svrCommPort;
      return true;
     }
     qInfo() << "node_stor: <Comm> Listening commands on port" << conf.svrCommPort;

     // Threads should be here
     recThread=new RecThread(&conf,this);
     recThread->start(QThread::HighestPriority);

     return false;
    } else {
     qWarning() << "node_stor: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "node_stor: The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

 private slots:
  void onNewCommClient() {
   while (commServer.hasPendingConnections()) {
    QTcpSocket *client=commServer.nextPendingConnection();
    client->setSocketOption(QAbstractSocket::LowDelayOption,1);
    client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray cmd=client->readAll().trimmed();
     handleCommand(QString::fromUtf8(cmd),client);
    });
    connect(client,&QTcpSocket::disconnected,client,&QObject::deleteLater);
    qInfo() << "node_stor: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; int iParam=0xffff; QIntValidator trigV(256,65535,this);
   qInfo() << "node-acq: <Comm> Received command:" << cmd;
   if (!cmd.contains("=")) {
    sList.append(cmd);
     qDebug("node-acq: <Comm> Sending Amp(s) status..");
     client->write("Amp(s) streaming EEG.\n");
    } else if (cmd==CMD_ACQ_DISCONNECT) { 
     qDebug("node-acq: <Comm> Disconnecting client..");
//     client->write("Disconnecting...\n");
//     client->disconnectFromHost();
    } else if (cmd==CMD_ACQ_GETCONF) {
     qDebug("node-acq: <Comm> Sending Config Parameters..");
//     client->write(QString::number(conf.ampCount).toUtf8()+","+ \
//                   QString::number(conf.eegRate).toUtf8()+","+ \
//                   QString::number(conf.refChnCount).toUtf8()+","+ \
//                   QString::number(conf.bipChnCount).toUtf8()+","+ \
//                   QString::number(conf.refGain).toUtf8()+","+ \
//                   QString::number(conf.bipGain).toUtf8()+","+ \
//                   QString::number(conf.eegProbeMsecs).toUtf8()+"\n");
    } else if (cmd==CMD_ACQ_GETCHAN) {
     qDebug("node-acq: <Comm> Sending Channels' Parameters..");
 //    for (const auto& ch:conf.chnInfo) {
 //     QString interMode="";
 //     for (int idx=0;idx<ch.interMode.size();idx++) interMode.append(QString::number(ch.interMode[idx])+",");
 //     interMode.remove(interMode.size()-1,1);
 //     client->write(QString::number(ch.physChn).toUtf8()+","+ \
 //                   QString(ch.chnName).toUtf8()+","+ \
 //                   QString::number(ch.topoTheta).toUtf8()+","+ \
 //                   QString::number(ch.topoPhi).toUtf8()+","+ \
 //                   QString::number(ch.topoX).toUtf8()+","+ \
 //                   QString::number(ch.topoY).toUtf8()+","+ \
 //                   QString::number(ch.type).toUtf8()+","+ \
 //                   interMode.toUtf8()+"\n");
 //    }
    } else if (cmd==CMD_STOR_DUMPON) {
//     static constexpr char dumpSign[]="OCTOPUS_HEEG"; // 12 bytes, no \0
//     QDateTime currentDT(QDateTime::currentDateTime());
//     QString tStamp=currentDT.toString("yyyyMMdd-hhmmss-zzz");
//     conf.hEEGFile.setFileName("/opt/octopus/heegdump-"+tStamp+".raw");
//     conf.hEEGFile.open(QIODevice::WriteOnly);
//     conf.hEEGStream.setDevice(&(conf.hEEGFile));
//     conf.hEEGStream.setVersion(QDataStream::Qt_5_15); // 5_12
//     conf.hEEGStream.setByteOrder(QDataStream::LittleEndian);
//     conf.hEEGStream.setFloatingPointPrecision(QDataStream::SinglePrecision); // 32-bit
//     conf.hEEGStream.writeRawData(dumpSign,sizeof(dumpSign)-1); // write signature *without* length or NUL
//     conf.hEEGStream << quint32(conf.ampCount) << quint32(conf.physChnCount);// << quint32(conf.audioCount);
//     conf.dumpRaw=true;
//     client->write("node-acq: Raw EEG dumping started.\n");
//    } else if (cmd==CMD_ACQ_DUMPOFF) {
//     conf.dumpRaw=false; conf.hEEGStream.setDevice(nullptr); conf.hEEGFile.close();
//     client->write("node-acq: Raw EEG dumping stopped.\n");
//    }
   } else { // command with parameter
    sList=cmd.split("=");
    if (sList[0]==CMD_ACQ_COMPCHAN) {
 //    QStringList params=sList[1].split(",");
 //    unsigned int amp=params[0].toInt()-1;
 //    unsigned int type=params[1].toInt();
 //    unsigned int chn=params[2].toInt();
 //    unsigned int state=params[3].toInt();
 //    {
 //     QMutexLocker locker(&conf.chnInterMutex);
 //     for (int chIdx=0;chIdx<conf.chnInfo.size();chIdx++) {
 //      if (conf.chnInfo[chIdx].type==type && conf.chnInfo[chIdx].physChn==chn) {
 //       conf.chnInfo[chIdx].interMode[amp]=state;
 //      }
 //     }
 //    }
     //conf.generateElecLists();
     //qDebug() << "node-acq: <Comm> Channel to be computed.. amp:" << amp+1 << "ctype:" << type << "chn:" << chn << "inter:" << state;
 //    QString response;
 //    for (int chIdx=0;chIdx<conf.chnInfo.size();chIdx++) {
 //     response+=QString::number(conf.chnInfo[chIdx].interMode[amp])+",";
 //    }
 //    response.remove(response.size()-1,1);
 //    client->write(response.toUtf8());
     //client->write("Channel to be computed.\n");
    } else {
     qDebug("node-acq: <Comm> Unknown command received..");
     client->write("Unknown command..\n");
    }
   }
  }

 private:
  QTcpServer commServer;
  RecThread *recThread;
  //QVector<ChnInfo> chnInfo;
};
