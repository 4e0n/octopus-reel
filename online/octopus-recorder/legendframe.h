/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef LEGENDFRAME_H
#define LEGENDFRAME_H

#include <QFrame>
#include <QPainter>

#include "octopus_rec_master.h"

class LegendFrame : public QFrame {
 Q_OBJECT
 public:
  LegendFrame(QWidget *pnt,RecMaster *rm) : QFrame(pnt) { parent=pnt; p=rm; }

 public slots:

 protected:
  virtual void paintEvent(QPaintEvent*) {
   mainPainter.begin(this);
   mainPainter.setBackground(QBrush(Qt::white));
   mainPainter.setBackgroundMode(Qt::OpaqueMode);
   mainPainter.setPen(Qt::black);
   mainPainter.eraseRect(frameRect()); mainPainter.drawRect(frameRect());
   mainPainter.drawText(16,16,"#   Event           Color"
                              "    Accepted  Rejected");
   mainPainter.drawLine(4,22,width()-4,22);
   for (int i=0;i<p->acqEvents.size();i++) {
    if (p->acqEvents[i]->accepted || p->acqEvents[i]->rejected) {
     noStr.setNum(p->acqEvents[i]->no);
     accStr.setNum(p->acqEvents[i]->accepted);
     rejStr.setNum(p->acqEvents[i]->rejected);
     mainPainter.setPen(Qt::black);
     mainPainter.drawText(8,45+i*20,noStr);
     mainPainter.drawText(30,45+i*20,p->acqEvents[i]->name);
     mainPainter.drawText(174,45+i*20,accStr);
     mainPainter.drawText(236,45+i*20,rejStr);
     QBrush eBrush(p->acqEvents[i]->color);
     mainPainter.fillRect(122,45+i*20-10,25,6,eBrush);
    }
   }
   mainPainter.end();
  }

 private:
  QWidget *parent; RecMaster *p; QString noStr,accStr,rejStr;
  QBrush bgBrush; QPainter mainPainter; QFont evtFont;
};

#endif
