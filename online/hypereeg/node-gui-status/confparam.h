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
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpSocket>
#include <QHostAddress>
#include <QStringList>
#include <atomic>
#include "../common/tcp_commands.h"

const int STATUS_REFRESH_MS=1000;
const int STATUS_REPLY_TIMEOUT_MS=500;
const int COMMAND_CONNECT_TIMEOUT_MS=1000;
const int COMMAND_REPLY_TIMEOUT_MS=1000;

struct MonitoredNode {
 QString name;
 QString ipAddr;
 quint32 commPort=0;
 bool online=false;
 bool checking=false;
 QString lastReply;
 qint64 lastRttMs=-1;
};

class ConfParam:public QObject {
 Q_OBJECT
 public:
  explicit ConfParam(QObject *parent=nullptr):QObject(parent) {
   statusRefreshMs=STATUS_REFRESH_MS;
   statusReplyTimeoutMs=STATUS_REPLY_TIMEOUT_MS;
   quitPending=false;
  }

  void requestQuit() { quitPending=true; }

  // Sends one command to one node using a temporary TCP socket.
  // Returns true if the command could be sent and the node replied.
  bool sendCommandToNode(const QString &ipAddr,quint32 port,const QString &cmd,
                         QString *replyOut=nullptr,QString *errOut=nullptr) {
   QTcpSocket sock; sock.connectToHost(ipAddr,port);

   if (!sock.waitForConnected(COMMAND_CONNECT_TIMEOUT_MS)) {
    if (errOut) *errOut="Connect failed to "+ipAddr+":"+QString::number(port)+" : "+sock.errorString();
    return false;
   }

   QByteArray payload=cmd.toUtf8();
   if (!payload.endsWith('\n')) payload.append('\n');

   qint64 wr=sock.write(payload);
   if (wr!=payload.size()) {
    if (errOut) *errOut="Write failed to "+ipAddr+":"+QString::number(port)+" : "+sock.errorString();
    sock.disconnectFromHost();
    return false;
   }

   if (!sock.waitForBytesWritten(COMMAND_REPLY_TIMEOUT_MS)) {
    if (errOut) *errOut="Timeout writing to "+ipAddr+":"+QString::number(port);
    sock.disconnectFromHost();
    return false;
   }

   if (!sock.waitForReadyRead(COMMAND_REPLY_TIMEOUT_MS)) {
    if (errOut) *errOut="No reply from "+ipAddr+":"+QString::number(port);
    sock.disconnectFromHost();
    return false;
   }

   QString reply=QString::fromUtf8(sock.readAll()).trimmed();
   if (replyOut) *replyOut=reply;

   sock.disconnectFromHost();
   sock.waitForDisconnected(250);

   return true;
  }

  // Broadcast helper for actions like reboot/shutdown
  QString broadcastCommand(const QString &cmd) {
   QStringList okList,failList;
   for (const auto &n:nodes) {
    QString reply,err;
    bool ok=sendCommandToNode(n.ipAddr,n.commPort,cmd,&reply,&err);
    if (ok) {
     if (reply.isEmpty()) okList << QString("%1 (%2)").arg(n.name,n.ipAddr);
     else okList << QString("%1 (%2): %3").arg(n.name,n.ipAddr,reply);
    } else {
     failList << QString("%1 (%2): %3").arg(n.name,n.ipAddr,err);
    }
   }

   QString out;
   out+="Command: "+cmd+"\n\n";

   out+="Succeeded:\n";
   if (okList.isEmpty()) out+="  none\n";
   else {
    for (const auto &s:okList) out+="  - "+s+"\n";
   }

   out+="\nFailed:\n";
   if (failList.isEmpty()) out+="  none\n";
   else {
    for (const auto &s:failList) out+="  - "+s+"\n";
   }

   return out;
  }

  QVector<MonitoredNode> nodes;

  int statusRefreshMs; int statusReplyTimeoutMs;

  int guiX=100; int guiY=100; int guiW=700; int guiH=300;

  std::atomic<bool> quitPending{false};
  QMutex mutex;
};
