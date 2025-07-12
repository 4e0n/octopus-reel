/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

#ifndef CONFPARAM_H
#define CONFPARAM_H

#include <QApplication>
#include <QColor>
#include <QTcpSocket>
#include "chninfo.h"
#include "../../../common/event.h"
#include "../tcpsample.h"

class ConfParam {
 public:
  ConfParam() {};
  void init(int ampC=0) { ampCount=ampC; tcpBufHead=tcpBufTail=0; eegAmpX.resize(ampCount); };

  QString commandToDaemon(const QString &command, int timeoutMs=1000) {
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  bool popSample(TcpSample& outSample) {
   if (tcpBufTail<tcpBufHead) { outSample=tcpBuffer[tcpBufTail%tcpBufSize]; tcpBufTail++; return true; }
   return false;
  }

  QTcpSocket *commSocket,*dataSocket;
  QVector<TcpSample> tcpBuffer; quint64 tcpBufHead,tcpBufTail; quint32 tcpBufSize;

  unsigned int ampCount,sampleRate,refChnCount,bipChnCount,eegProbeMsecs; float refGain,bipGain;
  QString ipAddr; quint32 commPort,dataPort;
  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH,guiStrmX,guiStrmY,guiStrmW,guiStrmH,guiHeadX,guiHeadY,guiHeadW,guiHeadH;
  int eegFrameW,eegFrameH,glFrameW,glFrameH;


  QVector<Event*> acqEvents;
  QVector<float> eegAmpX;
  QVector<ChnInfo> chns;
  QString curEventName;
  quint32 curEventType;
  float rejBwd,avgBwd,avgFwd,rejFwd; // In terms of milliseconds
  int cntPastSize,cntPastIndex; //,avgCount,rejCount,postRejCount,bwCount;
  QVector<QColor> rgbPalette;
  QVector<Event> events;

  QApplication *application;
};

#endif
