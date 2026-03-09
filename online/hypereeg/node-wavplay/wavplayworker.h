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
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMutex>

#include "confparam.h"

class WavPlayWorker : public QObject {
 Q_OBJECT
 public:
  explicit WavPlayWorker(ConfParam* c,QObject* parent=nullptr):QObject(parent),conf(c) {
   player=new QProcess(this);

   connect(player,&QProcess::stateChanged,this,
           [this](QProcess::ProcessState st) {
            QMutexLocker lk(&stMx);
            playing=(st!=QProcess::NotRunning);
            if (!playing) currentFile.clear();
           }
          );

   connect(player,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,
          [this](int exitCode,QProcess::ExitStatus exitStatus) {
           QMutexLocker lk(&stMx);
           playing=false;
           currentFile.clear();
           qInfo() << "<WavPlay> finished exitCode:" << exitCode << "exitStatus:" << exitStatus;
          }
         );

   connect(player,&QProcess::errorOccurred,this,
          [this](QProcess::ProcessError e) {
           QMutexLocker lk(&stMx);
           playing=false;
           currentFile.clear();
           qWarning() << "<WavPlay> QProcess error:" << e << (player ? player->errorString():QString());
          }
         );
  }

 public slots:
  void playAbsFile(const QString& absPath) {
   stop(); // restart behavior
   QFileInfo fi(absPath);
   if (!fi.exists()||!fi.isFile()) { qWarning() << "<WavPlay> file not found:" << absPath; return; }
   QStringList args;
   args << "-D" << conf->alsaDev << fi.absoluteFilePath();
   player->start("aplay",args);
   if (!player->waitForStarted(200)) {
    qWarning() << "<WavPlay> Failed to start gst-launch-1.0:" << player->errorString(); return;
   }
   {
     QMutexLocker lk(&stMx);
     currentFile=fi.fileName();
     playing=true; // stateChanged will correct if it exits
   }
   qInfo() << "<WavPlay> PLAY" << fi.fileName();
  }

  void stop() {
   if (player->state()!=QProcess::NotRunning) {
    qInfo() << "<WavPlay> STOP";
    player->terminate();
    if (!player->waitForFinished(300)) {
     player->kill();
     player->waitForFinished(300);
    }
   }
   QMutexLocker lk(&stMx);
   playing=false;
   currentFile.clear();
  }

  QString statusString() {
   QMutexLocker lk(&stMx);
   // e.g. "PLAYING=test.wav" or "IDLE"
   if (playing) return QString("PLAYING=%1").arg(currentFile);
   return "IDLE";
  }

 public:
  bool isPlaying() const { return playing; }
  QString nowPlaying() const { return currentFile; }

 private:
  ConfParam* conf=nullptr;
  QProcess *player=nullptr;
  bool playing=false;
  QString currentFile;
  mutable QMutex stMx;
};
