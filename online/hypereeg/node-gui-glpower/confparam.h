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

#include <QTcpSocket>
#include <QTcpServer>
#include <QMutex>
#include <atomic>
#include <QVector>
#include <QLabel>
#include "powchninfo.h"

const int GLPOWER_REFRESH_MS=500; // Not to be smaller than 500ms

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() { powRefreshMs=GLPOWER_REFRESH_MS; quitPending=false; }

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) { // Upstream command
   if (!socket || socket->state()!=QAbstractSocket::ConnectedState) return QString(); // or the error msg
   socket->write(command.toUtf8()+"\n");
   if (!socket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!socket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(socket->readAll()).trimmed();
  }

  void requestQuit() { 
   {
     QMutexLocker locker(&mutex);
     quitPending=true;
   }
  }

  unsigned int guiMaxAmpPerLine=0;

  QString compPPIpAddr; quint32 compPPCommPort; QTcpSocket *compPPCommSocket;

  unsigned int ampCount,refChnCount,bipChnCount,metaChnCount;
  unsigned int physChnCount,totalChnCount;

  unsigned int powerChnCount=0;
  unsigned int gfpAllIdx=0;
  unsigned int gfpLeftIdx=0;
  unsigned int gfpRightIdx=0;

  QVector<PowChnInfo> refChns,bipChns,metaChns;
  std::atomic<bool> quitPending{false};

  int powRefreshMs;
#ifdef EEGBANDSCOMP
  unsigned int eegBand;
#endif

  int guiX,guiY,guiW,guiH,frameW,frameH,cellSize;

  static constexpr int POWER_BAND_COUNT=6;
  static constexpr int POWER_OVERALL=0;
  static constexpr int POWER_DELTA=1;
  static constexpr int POWER_THETA=2;
  static constexpr int POWER_ALPHA=3;
  static constexpr int POWER_BETA=4;
  static constexpr int POWER_GAMMA=5;

  QVector<QVector<QVector<float>>> curPowData; // [amp][powerChn][band]

  QMutex mutex;

  quint32 powCommPort=0;
  QTcpServer powCommServer;
  QVector<QTcpSocket*> powClients;

  bool pollingActive=true;

 signals:

 public slots:

 private slots:

 private:
};
