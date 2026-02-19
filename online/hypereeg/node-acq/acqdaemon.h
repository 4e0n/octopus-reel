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
#include "../common/globals.h"
#include "../common/sample.h"
#include "../common/tcpsample.h"
#include "../common/tcp_commands.h"
#include "acqthread.h"
#include "tcpthread.h"

class AcqDaemon : public QObject {
 Q_OBJECT
 public:
  explicit AcqDaemon(QObject *parent=nullptr,ConfParam *c=nullptr,
                     std::vector<amplifier*>* ee=nullptr,AudioAmp *aud=nullptr,SerialDevice *ser=nullptr) : QObject(parent) {
   conf=c;
   // Downstream (we're server)
   connect(&(conf->acqCommServer),&QTcpServer::newConnection,this,&AcqDaemon::onNewCommClient);
   connect(&(conf->acqStrmServer),&QTcpServer::newConnection,this,&AcqDaemon::onNewStrmClient);
   acqThread=new AcqThread(conf,this,ee,aud,ser); // Thread for acquiring/packaging EEG data from amps+Audio data (Producer)
   tcpThread=new TcpThread(conf,this); // Thread for serving the packages from ringbuffer to connected clients (Consumer)
   connect(tcpThread,&TcpThread::sendPacketSignal,this,&AcqDaemon::sendPacket,Qt::QueuedConnection); // Send packets timely

   tcpThread->setStackSize(8*1024*1024); // 8 MiB
   acqThread->start(QThread::HighestPriority);
   tcpThread->start(QThread::HighestPriority);
  }

  ConfParam *conf;

 private slots:
  void onNewCommClient() {
   while (conf->acqCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->acqCommServer.nextPendingConnection();
    //client->setSocketOption(QAbstractSocket::LowDelayOption,1);
    //client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,60*1024);
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray cmd=client->readAll().trimmed();
     handleCommand(QString::fromUtf8(cmd),client);
    });
    connect(client,&QTcpSocket::disconnected,client,&QObject::deleteLater);
    qInfo() << "<Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   QStringList sList; int iParam=0xffff; QIntValidator trigV(256,65535,this);
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
                   QString::number(conf->frameBytes).toUtf8()+"\n");
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
    } else if (cmd==CMD_ACQ_DUMPON) {
     static constexpr char dumpSign[]="OCTOPUS_HEEG"; // 12 bytes, no \0
     QDateTime currentDT(QDateTime::currentDateTime());
     QString tStamp=currentDT.toString("yyyyMMdd-hhmmss-zzz");
     conf->hEEGFile.setFileName("/opt/octopus/data/raweeg/heegdump-"+tStamp+".raw");
     conf->hEEGFile.open(QIODevice::WriteOnly);
     conf->hEEGStream.setDevice(&(conf->hEEGFile));
     conf->hEEGStream.setVersion(QDataStream::Qt_5_15); // 5_12
     conf->hEEGStream.setByteOrder(QDataStream::LittleEndian);
     conf->hEEGStream.setFloatingPointPrecision(QDataStream::SinglePrecision); // 32-bit
     conf->hEEGStream.writeRawData(dumpSign,sizeof(dumpSign)-1); // write signature *without* length or NUL
     conf->hEEGStream << quint32(conf->ampCount) << quint32(conf->physChnCount);// << quint32(conf.audioCount);
     conf->dumpRaw=true;
     client->write("node-acq: Raw EEG dumping started.\n");
    } else if (cmd==CMD_ACQ_DUMPOFF) {
     conf->dumpRaw=false; conf->hEEGStream.setDevice(nullptr); conf->hEEGFile.close();
     client->write("node-acq: Raw EEG dumping stopped.\n");
    } else if (cmd==CMD_ACQ_AMPSYNC) {
     qInfo() << "<Comm> Conveying SYNC to amplifier(s)..";
     conf->syncOngoing=true; conf->syncPerformed=false;
     acqThread->sendTrigger(TRIG_AMPSYNC);
     client->write("SYNC conveyed to amps.\n");
    } else if (cmd==CMD_ACQ_S_TRIG_1000) {
     acqThread->sendTrigger(1000);
    } else if (cmd==CMD_ACQ_S_TRIG_1) {
     acqThread->sendTrigger(1);
    } else if (cmd==CMD_ACQ_S_TRIG_2) {
     acqThread->sendTrigger(2);
    }
   } else { // command with parameter
    sList=cmd.split("=");
    if (sList[0]=="TRIGGER") {
     int pos=0;
     if (trigV.validate(sList[1],pos)==QValidator::Acceptable) iParam=sList[1].toInt();
     if (iParam<0xffff) {
      qInfo("<Comm> Conveying **non-hardware** trigger to amplifier(s).. TCode:%d",iParam);
      acqThread->sendTrigger(iParam);
      client->write("**Non-hardware** trigger conveyed to amps.\n");
     } else {
      qWarning() << "<Comm> Error! **Non-hardware** trigger is out of (256,65535] range. Trigger not conveyed.";
      client->write("node-acq: <Comm> Error! **Non-hardware** trigger is out of (256,65535] range. Trigger not conveyed.\n");
     }
    } else if (sList[0]==CMD_ACQ_COMPCHAN) {
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
   while (conf->acqStrmServer.hasPendingConnections()) {
    QTcpSocket *client=conf->acqStrmServer.nextPendingConnection();
    
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
   }
  }

 private:
  QVector<QTcpSocket*> strmClients;
  AcqThread *acqThread; TcpThread *tcpThread;
};
