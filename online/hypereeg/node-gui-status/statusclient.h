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
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include "../common/globals.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "statuswindow.h"

class StatusClient:public QObject {
 Q_OBJECT
 public:
  explicit StatusClient(QObject *parent=nullptr,ConfParam *c=nullptr):QObject(parent) { conf=c; }

  bool start() {
   pollTimer=new QTimer(this);
   connect(pollTimer,&QTimer::timeout,this,&StatusClient::slotPoll);

   statusWindow=new StatusWindow(conf); statusWindow->show();

   pollTimer->start(conf->statusRefreshMs); slotPoll();

   return false;
  }

  ConfParam *conf=nullptr;

 private slots:
  void slotPoll() {
   for (int i=0;i<conf->nodes.size();i++) { pollNode(i); }
   if (statusWindow) statusWindow->refreshTable();
  }

 private:
  void pollNode(int idx) {
   if (idx<0 || idx>=conf->nodes.size()) return;

   auto &n=conf->nodes[idx];
   n.checking=true; n.online=false; n.lastReply.clear(); n.lastRttMs=-1;

   QTcpSocket sock; QElapsedTimer timer; timer.start();

   sock.connectToHost(n.ipAddr,n.commPort);
   if (!sock.waitForConnected(conf->statusReplyTimeoutMs)) {
    n.checking=false; n.online=false;
    n.lastReply=sock.errorString();
    return;
   }

   sock.write(QString(CMD_STATUS+"\n").toUtf8());
   if (!sock.waitForBytesWritten(conf->statusReplyTimeoutMs)) {
    n.checking=false; n.online=false; n.lastReply="write timeout";
    sock.abort();
    return;
   }

   if (!sock.waitForReadyRead(conf->statusReplyTimeoutMs)) {
    n.checking=false; n.online=false; n.lastReply="reply timeout";
    sock.abort();
    return;
   }

   QString reply=QString::fromUtf8(sock.readAll()).trimmed();
   n.lastReply=reply; n.lastRttMs=timer.elapsed(); n.online=(reply=="OK"); n.checking=false;

   sock.disconnectFromHost();
  }

  StatusWindow *statusWindow=nullptr;
  QTimer *pollTimer=nullptr;
};
