/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef STATSEG_W_H
#define STATSEG_W_H

#include <QtGui>
#include "octopus_seg_master.h"

class StatSegW : public QWidget {
 Q_OBJECT
 public:
  StatSegW(SegMaster *sm) : QWidget(0,Qt::SubWindow) {
   p=sm; setGeometry(540,40,200,100); setFixedSize(200,100);

   centroidLabel=new QLabel("Centroid Count ("+
                            dummyString.setNum(p->kMeansCCount)+"):",this);
   centroidLabel->setGeometry(10,10,190,20);

   QSlider *centroidSlider=new QSlider(Qt::Horizontal,this);
   centroidSlider->setGeometry(10,30,180,20);
   centroidSlider->setRange(0,9);
   centroidSlider->setSingleStep(1);
   centroidSlider->setPageStep(1);
   centroidSlider->setSliderPosition(p->kMeansCCount-1);
   centroidSlider->setEnabled(true);
   connect(centroidSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetKMeansCCount(int)));

   setWindowTitle("Statistical Segmentation");
  }

 private slots:
  void slotSetKMeansCCount(int x) { p->kMeansCCount=x+1;
   centroidLabel->setText("Centroid Count ("+
                          dummyString.setNum(x+1)+"):");
  }

 private:
  SegMaster *p; QLabel *centroidLabel; QString dummyString;
};

#endif


