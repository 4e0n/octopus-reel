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
#include <QTcpSocket>
#include <QString>
#include <QVector>
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QIntValidator>
#include <QThread>
#include <QProcess>
#include "../common/globals.h"
#include "../common/tcp_commands.h"
#include "wavplayworker.h"

class WavPlayDaemon: public QObject {
 Q_OBJECT
 public:
  explicit WavPlayDaemon(QObject *parent=nullptr,ConfParam *c=nullptr):QObject(parent) { conf=c; }

  ~WavPlayDaemon() override {
   if (wavPlayWorker) QMetaObject::invokeMethod(wavPlayWorker,"stop",Qt::BlockingQueuedConnection);
   audioThread.quit();
   audioThread.wait(500);
  }

  bool start() { QString commResponse; QStringList sList,sList2;

   if (!conf->wavPlayCommServer.listen(QHostAddress::Any,conf->wavPlayCommPort)) {
    qCritical() << "<Comm> Cannot start TCP server on <Comm> port:" << conf->wavPlayCommPort;
    return true;
   }

   // We're server
   connect(&(conf->wavPlayCommServer),&QTcpServer::newConnection,this,&WavPlayDaemon::onNewCommClient);

   // Threads
   wavPlayWorker=new WavPlayWorker(conf);
   wavPlayWorker->moveToThread(&audioThread);
   connect(&audioThread,&QThread::finished,wavPlayWorker,&QObject::deleteLater);
   audioThread.start();

   return false;
  }

  ConfParam *conf;

 private slots:
  void onNewCommClient() {
   while (conf->wavPlayCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->wavPlayCommServer.nextPendingConnection();
    //client->setSocketOption(QAbstractSocket::LowDelayOption,1);
    //client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
 
    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     while (client->canReadLine()) {
      const QByteArray cmd=client->readLine().trimmed(); // trims \r\n too
      if (!cmd.isEmpty()) handleCommand(QString::fromUtf8(cmd),client);
     }
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
    if (cmd==CMD_DISCONNECT) { 
     qInfo() << "<Comm> Disconnecting client..";
     client->write("Disconnecting...\n");
     client->disconnectFromHost();
    } else if (cmd==CMD_WAVPLAY_STOP) {
     QMetaObject::invokeMethod(wavPlayWorker,"stop",Qt::QueuedConnection);
     client->write("OK\n");
    } else if (cmd==CMD_WAVPLAY_LIST) {
     QDir d(conf->wavDir);
     QStringList files=d.entryList(QStringList() << "*.wav" << "*.WAV", QDir::Files, QDir::Name);
     client->write(files.join("\n").toUtf8());
     client->write("\n");
    } else if (cmd==CMD_STATUS) {
     QString st;
     QMetaObject::invokeMethod(wavPlayWorker,"statusString",Qt::BlockingQueuedConnection,Q_RETURN_ARG(QString,st));
     //invoke worker -> st (IDLE or PLAYING=file)
     QString resp=QString("%1 DEV=%2 DIR=%3").arg(st).arg(conf->alsaDev).arg(conf->wavDir);
     client->write(resp.toUtf8()); client->write("\n");
    } else if (cmd==CMD_QUIT) {
     client->write("Bye!\r\n");
     client->disconnectFromHost();
    } else if (cmd==CMD_REBOOT) {
     const int delaySec=5;
     const QString cmd=QString("sleep %1; /usr/bin/systemctl reboot -i").arg(delaySec);
     //QProcess::startDetached("/bin/sh",QStringList() << "-c" << cmd);
     bool ok=QProcess::startDetached("/bin/sh", {"-c",cmd});
     client->disconnectFromHost();
    } else if (cmd==CMD_SHUTDOWN) {
     const int delaySec=5;
     const QString cmd=QString("sleep %1; /usr/bin/systemctl poweroff -i").arg(delaySec);
     //QProcess::startDetached("/bin/sh",QStringList() << "-c" << cmd);
     bool ok=QProcess::startDetached("/bin/sh", {"-c",cmd});
     client->disconnectFromHost();
    }
   } else { // command with parameter
    sList=cmd.split("=");
    if (sList[0]==CMD_WAVPLAY_PLAY) {
     const QString leaf=sList[1].trimmed();
     if (!isSafeLeaf(leaf)) { client->write("ERR bad_filename\n"); return; }
     const QString abs=QDir(conf->wavDir).absoluteFilePath(leaf);
     if (!QFileInfo::exists(abs)) { client->write("ERR not_found\n"); return; }
     QMetaObject::invokeMethod(wavPlayWorker, "playAbsFile", Qt::QueuedConnection,
                               Q_ARG(QString, abs));
     client->write("OK\n");
    } else if (sList[0]==CMD_WAVPLAY_SETSINK) {
     const QString dev=sList[1].trimmed();
     if (dev.isEmpty()) { client->write("ERR bad_dev\n"); return; }
     conf->alsaDev=dev;
     client->write("OK\n");
    } else {
     qWarning() << "<Comm> Unknown command received..";
     client->write("node-wavplay: Unknown command..\n");
    }
   }
  }

 private:
  static bool isSafeLeaf(const QString& s) {
   if (s.isEmpty()) return false;
   if (s.contains('/') || s.contains('\\')) return false;
   if (s.contains("..")) return false;
   if (!s.endsWith(".wav",Qt::CaseInsensitive)) return false;
   return true;
  }
  QVector<QTcpSocket*> strmClients;
  QThread audioThread; WavPlayWorker* wavPlayWorker=nullptr;
};
