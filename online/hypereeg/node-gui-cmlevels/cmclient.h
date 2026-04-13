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

#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QString>
#include <QVector>
#include <QTimer>
#include <QByteArray>
#include "../common/globals.h"
#include "../common/tcpsample_pp.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
#include "cmwindow.h"

const int CMFRAME_REFRESH_RATE=1; // Base refresh rate (seconds)

const int GUI_MAX_AMP_PER_LINE=4; // If >4 then (4,2), otherwise (N,1)

const int CM_REPLY_TIMEOUT_MS=1500;

class CMClient: public QObject {
 Q_OBJECT
 public:
  explicit CMClient(QObject *parent=nullptr,ConfParam *c=nullptr) : QObject(parent) { conf=c; }

  bool start() { QString commResponse; QStringList sList,sList2;

   // We're client of node-comp-pp
   conf->compPPCommSocket=new QTcpSocket(this);
   conf->compPPCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf->compPPCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);

   // Setup COMPPP command socket
   conf->compPPCommSocket->connectToHost(conf->compPPIpAddr,conf->compPPCommPort);
   if (!conf->compPPCommSocket->waitForConnected(2000)) {
    qCritical() << "node-gui-cmlevels: <ConfigParser> Cannot connect to node-comp-pp:" << conf->compPPCommSocket->errorString();
    return true;
   }
   // Get crucial info from the "acquisition" node we connect to
   commResponse=conf->commandToDaemon(conf->compPPCommSocket,CMD_ACQ_GETCONF);
   if (commResponse.isEmpty()) {
    qCritical() << "node-gui-cmlevels: <ConfigParser> No response from Acquisition Node!";
    return true;
   }
   sList=commResponse.split(",");
   if (sList.size()<9) { // or 13 if you expect full GETCONF payload
    qCritical() << "node-gui-cmlevels: <ConfigParser> Bad GETCONF reply:" << commResponse;
    return true;
   }
   conf->ampCount=sList[0].toInt(); // (ACTUAL) AMPCOUNT
   conf->refChnCount=sList[2].toInt();
   conf->bipChnCount=sList[3].toInt();
   conf->metaChnCount=sList[4].toInt();
   conf->physChnCount=sList[5].toInt();
   conf->chnCount=sList[6].toInt();
   conf->totalChnCount=sList[7].toInt();
   conf->totalCount=sList[8].toInt();

   // CHANNELS
   const auto refChnCount=conf->refChnCount; const auto bipChnCount=conf->bipChnCount;
   const auto metaChnCount=conf->metaChnCount; const auto physChnCount=conf->physChnCount;

   commResponse=conf->commandToDaemon(conf->compPPCommSocket,CMD_ACQ_GETCHAN);
   if (commResponse.isEmpty()) qCritical() << "node-gui-cmlevels: <GetChannelListFromDaemon> (TIMEOUT) No response from Node!";
   sList=commResponse.split("\n"); GUIChnInfo chn;

   conf->refChns.clear();
   for (unsigned int chnIdx=0;chnIdx<refChnCount;chnIdx++) { // Individual CHANNELs information
    sList2=sList[chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    conf->refChns.append(chn);
   }
   conf->bipChns.clear();
   for (unsigned int chnIdx=0;chnIdx<bipChnCount;chnIdx++) { // Individual CHANNELs information
    sList2=sList[refChnCount+chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    conf->bipChns.append(chn);
   }
   conf->metaChns.clear();
   for (unsigned int chnIdx=0;chnIdx<metaChnCount;chnIdx++) { // Individual CHANNELs information
    sList2=sList[physChnCount+chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    conf->metaChns.append(chn);
   }

   conf->guiMaxAmpPerLine=GUI_MAX_AMP_PER_LINE;

   pollTimer=new QTimer(this);
   connect(pollTimer,&QTimer::timeout,this,&CMClient::slotPoll);

   replyTimer=new QTimer(this);replyTimer->setSingleShot(true);
   connect(replyTimer,&QTimer::timeout,this,&CMClient::slotReplyTimeout);

   connect(&(conf->cmCommServer),&QTcpServer::newConnection,this,&CMClient::slotNewCommClient);

   connect(conf->compPPCommSocket,&QTcpSocket::readyRead,this,&CMClient::slotReadyRead);
   connect(conf->compPPCommSocket,&QTcpSocket::disconnected,this,&CMClient::slotDisconnected);

   if (!conf->cmCommServer.listen(QHostAddress::Any,conf->cmCommPort)) {
    qCritical() << "node-gui-cmlevels: <Comm> Cannot start command server on port:" << conf->cmCommPort;
    return true;
   }

   // Constants or calculated global settings upon the ones read from config file
   if (conf->ampCount<=conf->guiMaxAmpPerLine) {
    conf->frameW=(conf->guiW-60)/conf->ampCount-10;
    conf->frameH=conf->guiH-60;
    conf->cellSize=conf->frameW/11;
   } else {
    const int perRow=conf->guiMaxAmpPerLine;
    const int rows=(conf->ampCount+perRow-1)/perRow;
    conf->frameW=(conf->guiW-40-(perRow-1)*20)/perRow;
    conf->frameH=(conf->guiH-40-(rows-1)*20)/rows;
    conf->cellSize=conf->frameW/11;
   }

   conf->curCMData.resize(conf->ampCount); for (auto& c:conf->curCMData) c.resize(conf->chnCount);

   cmWindow=new CMWindow(conf); cmWindow->show(); // CM Levels Window
   pollTimer->start(conf->cmRefreshMs);

   return false;
  }

  ConfParam *conf;

 private slots:
  void slotNewCommClient() {
   while (conf->cmCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->cmCommServer.nextPendingConnection();
    conf->cmClients.append(client);

    //connect(client,&QTcpSocket::readyRead,this,[this,client]() {
    // QByteArray cmd=client->readAll().trimmed();
    // handleCommand(QString::fromUtf8(cmd),client);
    //});
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray buf=client->property("rxbuf").toByteArray();
     buf+=client->readAll();
     int nlIdx;
     while ((nlIdx=buf.indexOf('\n'))>=0) {
      QByteArray line=buf.left(nlIdx).trimmed();
      buf.remove(0,nlIdx+1);
      if (!line.isEmpty()) handleCommand(QString::fromUtf8(line),client);
     }
     client->setProperty("rxbuf",buf);
    });

    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     conf->cmClients.removeAll(client);
     client->deleteLater();
    });

    qInfo() << "node-gui-cmlevels: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   qInfo() << "node-gui-cmlevels: <Comm> Received command:" << cmd;
   if (cmd==CMD_STATUS) {
    client->write("node-gui-cmlevels: ready.\n");
   } else if (cmd==CMD_GUI_SHOW) {
    if (cmWindow) cmWindow->show();
    client->write("node-gui-cmlevels: gui shown.\n");
   } else if (cmd==CMD_GUI_HIDE) {
    if (cmWindow) cmWindow->hide();
    client->write("node-gui-cmlevels: gui hidden.\n");
   } else if (cmd==CMD_GUI_RAISE) {
    if (cmWindow) {
     cmWindow->show(); cmWindow->raise(); cmWindow->activateWindow();
    }
    client->write("node-gui-cmlevels: gui raised.\n");
   } else if (cmd==CMD_GUI_REFRESH) {
    if (cmWindow) cmWindow->updateCMFrames();
    client->write("node-gui-cmlevels: refreshed.\n");
   } else if (cmd==CMD_GUI_START) {
    conf->pollingActive=true;
    if (pollTimer && !pollTimer->isActive()) pollTimer->start(conf->cmRefreshMs);
    client->write("node-gui-cmlevels: polling started.\n");
   } else if (cmd.startsWith(CMD_GUI_SETREFRESH)) {
    QStringList parts=cmd.split("=");
    if (parts.size()==2) {
     int ms=parts[1].toInt();
     if (ms>=50 && ms<=5000) {
      conf->cmRefreshMs=ms;
      if (pollTimer->isActive()) {
       pollTimer->stop();
       pollTimer->start(conf->cmRefreshMs);
      }
      client->write(QString("node-gui-cmlevels: refresh set to %1 ms\n").arg(ms).toUtf8());
     } else {
      client->write("node-gui-cmlevels: invalid refresh range (50–5000 ms)\n");
     }
    } else {
     client->write("node-gui-cmlevels: usage GUISETREFRESH=ms\n");
    }
   } else if (cmd.startsWith(CMD_GUI_PALETTE)) {
    QStringList parts=cmd.split("=");
    if (parts.size()==2) {
     QString mode=parts[1].trimmed().toUpper();
     if (mode=="MEAN" || mode=="MEDIAN") {
      if (cmWindow) cmWindow->setPaletteMode(mode);
      client->write(QString("node-gui-cmlevels: palette set to %1\n").arg(mode).toUtf8());
     } else {
      client->write("node-gui-cmlevels: use MEAN or MEDIAN\n");
     }
    } else {
     client->write("node-gui-cmlevels: usage GUIPALETTE=MEAN|MEDIAN\n");
    }
   } else if (cmd==CMD_GUI_STOP) {
    conf->pollingActive=false;
    if (pollTimer && pollTimer->isActive()) pollTimer->stop();
    client->write("node-gui-cmlevels: polling stopped.\n");
   } else if (cmd==CMD_QUIT) {
    client->write("node-gui-cmlevels: quitting.\n");
    client->flush();
    requestShutdown(); // graceful shutdown
   } else {
    client->write("node-gui-cmlevels: unknown command.\n");
   }
  }

  void slotPoll() {
   if (!conf->pollingActive) return;
   if (!conf->compPPCommSocket) return;
   if (conf->compPPCommSocket->state()!=QAbstractSocket::ConnectedState) return;
   if (requestPending) return;

   conf->compPPCommSocket->write(QString(CMD_COMPPP_GETCMLEVELS+QString("\n")).toUtf8());
   //conf->compPPCommSocket->write(QByteArray(CMD_COMPPP_GETCMLEVELS)+"\n");
   //conf->compPPCommSocket->flush();
   requestPending=true; replyTimer->start(CM_REPLY_TIMEOUT_MS);
   ++pollCounter;
  }

  void slotReadyRead() {
   rxBuffer+=conf->compPPCommSocket->readAll();
   int nlIdx;
   while ((nlIdx=rxBuffer.indexOf('\n'))>=0) {
    QByteArray line=rxBuffer.left(nlIdx).trimmed();
    rxBuffer.remove(0,nlIdx+1);
    if (line.isEmpty()) continue;
    QString reply=QString::fromUtf8(line);
    QStringList vals=reply.split(",",Qt::SkipEmptyParts);
    const int expected=int(conf->ampCount)*int(conf->chnCount);
    if (vals.size()!=expected) {
     qWarning() << "node-gui-cmlevels: <Parse> Bad reply size:" << vals.size() << "expected:" << expected;
     requestPending=false;
     if (replyTimer->isActive()) replyTimer->stop();
     continue;
    }
    int k=0;
    for (unsigned int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) {
     for (unsigned int chnIdx=0;chnIdx<conf->chnCount;chnIdx++) {
      conf->curCMData[ampIdx][chnIdx]=vals[k].toFloat();
      k++;
     }
    }
    requestPending=false;
    if (replyTimer->isActive()) replyTimer->stop();
    if (cmWindow) cmWindow->updateCMFrames();
   }
  }

  void slotReplyTimeout() {
   if (!requestPending) return;
   qWarning() << "node-gui-cmlevels: <PollTimeout> No CMLEVELS reply within timeout.";
   requestPending=false;
   // For current simple line-based protocol this is acceptable.
   rxBuffer.clear();
  }

  void slotDisconnected() {
   qWarning() << "node-gui-cmlevels: <Socket> Disconnected from node-comp-pp.";
   requestPending=false;
   rxBuffer.clear();
   if (replyTimer->isActive()) replyTimer->stop();
  }

 private:
  void requestShutdown() {
   qInfo() << "node-gui-cmlevels: <Shutdown> Requested.";
   conf->quitPending=true; conf->cmCommServer.close();
   if (pollTimer && pollTimer->isActive()) pollTimer->stop(); // stop polling
   requestPending=false;
   if (replyTimer && replyTimer->isActive()) replyTimer->stop(); // stop reply timer
   if (conf->compPPCommSocket) conf->compPPCommSocket->disconnectFromHost(); // disconnect upstream
   const auto clients=conf->cmClients;
   for (auto *c:clients) { // disconnect all command clients
    if (c) { c->write("node-gui-cmlevels: server shutting down.\n"); c->flush(); c->disconnectFromHost(); }
   }
   conf->cmClients.clear();
   QTimer::singleShot(0,[](){ QCoreApplication::quit(); }); // quit event loop (safe, no qApp needed)
  }

  CMWindow *cmWindow=nullptr;
  QTimer *pollTimer=nullptr,*replyTimer=nullptr;
  QByteArray rxBuffer; bool requestPending=false;
  quint64 pollCounter=0;
};
