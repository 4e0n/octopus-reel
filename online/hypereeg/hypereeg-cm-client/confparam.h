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
#include <QMutex>
#include <atomic>
#include <QVector>
#include "chninfo.h"
#include "../../../common/event.h"
#include "../tcpcmarray.h"

const unsigned int GUI_MAX_AMP_PER_LINE=4;

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {};
  void init(int ampC=0) {
   ampCount=ampC; tcpBufHead=tcpBufTail=0; updateInstant=false;
   updated.resize(ampCount); cmCurData.resize(ampCount);
   guiMaxAmpPerLine=GUI_MAX_AMP_PER_LINE;
  };

  QString commandToDaemon(const QString &command,int timeoutMs=1000) {
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  QTcpSocket *commSocket,*dataSocket;
  QVector<TcpCMArray> tcpBuffer; quint64 tcpBufHead,tcpBufTail; quint32 tcpBufSize;

  unsigned int ampCount,cmRate,refChnCount,bipChnCount,chnCount;
  unsigned int guiMaxAmpPerLine;
  QString ipAddr; quint32 commPort,dataPort; // dataPort is cmDataPort
  int guiX,guiY,guiW,guiH,frameW,frameH,guiCellSize;

  QVector<QVector<unsigned char> > cmCurData;

  QVector<ChnInfo> chns;
  QVector<QColor> rgbPalette;

  QApplication *application;

  std::atomic<bool> updateInstant; QVector<bool> updated;
  mutable QMutex mutex;

 public slots:
  void onDataReady() {
   static QByteArray buffer; buffer.append(dataSocket->readAll());
   //qDebug() << "[Client] Incoming data size:" << buffer.size();
   while (buffer.size()>=4) {
    QDataStream sizeStream(buffer); sizeStream.setByteOrder(QDataStream::LittleEndian);
    quint32 blockSize=0; sizeStream>>blockSize;
    //qDebug() << "[Client] Expected blockSize:" << blockSize;
    if ((quint32)buffer.size()<4+blockSize) break; // Wait for more data
    QByteArray block=buffer.mid(4,blockSize);
    // Deserialize into TcpSample
    TcpCMArray tcpCM;
    if (tcpCM.deserialize(block,chnCount)) {
     // Push the deserialized tcpSample to circular buffer
     tcpBuffer[tcpBufHead%tcpBufSize]=tcpCM; tcpBufHead++;
     //qDebug() << "CM Received!";
    } else {
     qWarning() << "Failed to deserialize TcpSample block.";
    }

    // Update
    for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
     updated[ampIdx]=false;
     for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++)
      cmCurData[ampIdx][chnIdx]=tcpCM.cmLevel[ampIdx][chnIdx];
    }
    tcpBufTail++;
    updateInstant=true;

    buffer.remove(0,4+blockSize); // Remove processed block
   }
  }
};

#endif
