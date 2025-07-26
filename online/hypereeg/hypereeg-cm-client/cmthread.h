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

#ifndef CMTHREAD_H
#define CMTHREAD_H

#include <QThread>
#include <QMutex>
#include <QBrush>
#include <QPainter>
#include <QStaticText>
#include <QVector>
#include <QBitmap>
#include <QMatrix>
#include <thread>
#include "confparam.h"

class CMThread : public QThread {
 Q_OBJECT
 public:
  explicit CMThread(ConfParam *c=nullptr,unsigned int a=0,QPixmap *cb=nullptr,QObject *parent=nullptr) : QThread(parent) {
   conf=c; ampNo=a; cmBuffer=cb;

   chnCount=conf->chns.size();
   hdrFont1=QFont("Helvetica",28,QFont::Bold);
   chnFont1=QFont("Helvetica",11,QFont::Bold);
   chnFont2=QFont("Helvetica",16,QFont::Bold);

   // Green <-> Yellow <-> Red
   for (unsigned int i=0;i<128;i++) palette.append(QColor(2*i,255,0));
   for (unsigned int i=0;i<128;i++) palette.append(QColor(255,255-2*i,0));
   //for (auto& p:palette) qDebug() << p;
  }

  void resetCMBuffer() {
   QRect cr(0,0,conf->frameW-1,conf->frameH-1);
   cmBuffer->fill(Qt::white);
   cmPainter.begin(cmBuffer);
    cmPainter.setPen(Qt::red);
    cmPainter.drawRect(cr);
    cmPainter.drawLine(0,0,conf->frameW-1,conf->frameH-1);
   cmPainter.end();
  }

  void updateBuffer() { int curCol;
   unsigned int chnIdx,topoX,topoY,a,y,sz; QString chnName;

   QRect cr(0,0,conf->frameW,conf->frameH);
   QPen pen1(Qt::black,4);
   QPen pen2(Qt::black,2);

   a=conf->guiCellSize; sz=5*a/6;

   //cmBuffer->fill(Qt::white);

   cmPainter.begin(cmBuffer);
    cmPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    cmPainter.setBackground(QBrush(Qt::white));
    cmPainter.setPen(Qt::black);
    // cmPainter.setBrush(Qt::black);
    cmPainter.fillRect(cr,QBrush(Qt::white));
    cmPainter.drawRect(cr);

    QString hdrString="EEG ";
    hdrString.append(QString::number(ampNo+1));
    cmPainter.setFont(hdrFont1);
    cmPainter.drawText(QRect(0,0,conf->frameW,a),Qt::AlignVCenter,hdrString);

    for (int i=0;i<(conf->chns).size();i++) {
     chnIdx=(conf->chns)[i].physChn; chnName=(conf->chns)[i].chnName;
     topoX=(conf->chns)[i].topoX; topoY=(conf->chns)[i].topoY;
     cmPainter.setPen(pen1);
     if (chnIdx<=32) cmPainter.setBrush(cmBrush(i));
     else if (chnIdx<=64) cmPainter.setBrush(cmBrush(i));
     else cmPainter.setBrush(cmBrush(i));
     y=0;
     if (topoY>1) y+=a/2;
     if (topoY>10) y+=a/2;
     QRect cr(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2,sz,sz);
     QRect tr1(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2-sz/4,sz,sz);
     QRect tr2(a/2+a*(topoX-1)-sz/2,y+a/2+a*(topoY-1)-sz/2+sz/6,sz,sz);
     //cmPainter.fillRect(cr,QBrush(Qt::gray));
     //cmPainter.drawRect(cr);
     cmPainter.drawEllipse(cr);
     cmPainter.setFont(chnFont1);
     cmPainter.drawText(tr1,Qt::AlignHCenter|Qt::AlignVCenter,QString::number(chnIdx));
     cmPainter.setFont(chnFont2);
     cmPainter.drawText(tr2,Qt::AlignHCenter|Qt::AlignVCenter,chnName);
    }
    cmPainter.setPen(pen2);
    cmPainter.drawLine(a/2,a,(conf->frameW)-a/2,a);
    cmPainter.drawLine(a/2,(conf->frameH)-a,(conf->frameW)-a/2,(conf->frameH)-a);
   cmPainter.end();
  }

 signals:
  void updateCMFrame();

 protected:
  virtual void run() override {
   while (true) {
    //QMutexLocker locker(&(conf->mutex));
    //conf->mutex.lock();
    if (conf->updateInstant) {
     if (!conf->updated[ampNo]) {
      updateBuffer();
      //for (unsigned int i=0;i<conf->chnCount;i++) qDebug("%d %d",ampNo,(conf->cmCurData)[ampNo][i]);
      //qDebug("\n");
      //qDebug() << (conf->cmCurData)[1][7];
      emit updateCMFrame();
      //qDebug() << "I am Thread" << ampNo;
      conf->updated[ampNo]=true;
     }
     //qDebug() << "Updating buffer";
    }
    bool allUpdated=true;
    for (unsigned int i=0;i<conf->ampCount;i++) if (conf->updated[i]==false) { allUpdated=false; break; }
    if (allUpdated){
    // qDebug() << "ALL UPDATED!";
     conf->updateInstant=false;
     //conf->mutex.unlock();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }
   qDebug("octopus_hacq_client: <CMThread> Exiting thread..");
  }

  QBrush cmBrush(int elec) {
   unsigned char cmLevel;
   cmLevel=(conf->cmCurData)[ampNo][elec];
   return palette[cmLevel];
  }

 private:
  ConfParam *conf; unsigned int ampNo; QPixmap *cmBuffer;
  int chnCount;
  QPainter cmPainter; //QVector<QStaticText> chnTextCache;
  QString dummyString;
  QFont hdrFont1,chnFont1,chnFont2;
  QVector<QColor> palette;
};

#endif
