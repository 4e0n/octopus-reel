/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef THRESHOLD_W_H
#define THRESHOLD_W_H

#include <QtGui>
#include "octopus_seg_master.h"

class ThresholdW : public QWidget {
 Q_OBJECT
 public:
  ThresholdW(SegMaster *sm) : QWidget(0,Qt::SubWindow) {
   p=sm; setGeometry(p->guiX+p->guiWidth+20,p->guiY,170,290);
   setFixedSize(170,290);

   thrLabel=new QLabel(dummyString.setNum(p->thrValue),this);
   thrLabel->setGeometry(10,10,30,20);

   QSlider *thrSlider=new QSlider(Qt::Vertical,this);
   thrSlider->setGeometry(12,30,20,256);
   thrSlider->setRange(0,255);
   thrSlider->setSingleStep(1);
   thrSlider->setPageStep(1);
   thrSlider->setSliderPosition(p->thrValue);
   thrSlider->setEnabled(true);
   connect(thrSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetThrValue(int)));

   QCheckBox *fillCheckBox=new QCheckBox("Filling",this);
   fillCheckBox->setGeometry(50,10,120,20);
   connect(fillCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotThrFillSet(int)));

   QCheckBox *binCheckBox=new QCheckBox("Binarization",this);
   binCheckBox->setGeometry(50,40,120,20);
   connect(binCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotThrBinSet(int)));

   QCheckBox *invCheckBox=new QCheckBox("Invert",this);
   invCheckBox->setGeometry(50,70,120,20);
   connect(invCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotThrInvSet(int)));

   filtLabel=new QLabel("Filt.Size ("+
                        dummyString.setNum(p->f3dRadius)+
                        "):",this);
   filtLabel->setGeometry(50,190,90,20);

   QSlider *filtSlider=new QSlider(Qt::Horizontal,this);
   filtSlider->setGeometry(50,220,80,20);
   filtSlider->setRange(0,9);
   filtSlider->setSingleStep(1);
   filtSlider->setPageStep(1);
   filtSlider->setSliderPosition(p->f3dRadius-1);
   filtSlider->setEnabled(true);
   connect(filtSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetFltValue(int)));

   setWindowTitle("Thresholding Settings");
  }

 private slots:
  void slotSetThrValue(int x) {
   thrLabel->setText(dummyString.setNum(x)); p->setThrValue(x);
  }
  void slotSetFltValue(int r) { p->setFltValue(r+1);
   filtLabel->setText("Filt.Size ("+
                      dummyString.setNum(r+1)+"):");
  }
  void slotThrFillSet(int x) { p->thrFill=(bool)x; }
  void slotThrBinSet(int x) { p->thrBin=(bool)x; }
  void slotThrInvSet(int x) { p->thrInv=(bool)x; }


 private:
  SegMaster *p; QLabel *thrLabel,*filtLabel; QString dummyString;
};

#endif
