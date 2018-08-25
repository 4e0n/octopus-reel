/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef REGGROW_W_H
#define REGGROW_W_H

#include <QtGui>
#include "octopus_seg_master.h"

class RegGrowW : public QWidget {
 Q_OBJECT
 public:
  RegGrowW(SegMaster *sm) : QWidget(0,Qt::SubWindow) {
   p=sm; setGeometry(p->guiX+p->guiWidth+20,p->guiY,170,190);
   setFixedSize(170,190);

   bndLabel=new QLabel(dummyString.setNum(p->floodBoundary),this);
   bndLabel->setGeometry(10,10,30,20);

   QSlider *bndSlider=new QSlider(Qt::Vertical,this);
   bndSlider->setGeometry(12,30,20,156);
   bndSlider->setRange(0,255);
   bndSlider->setSingleStep(1);
   bndSlider->setPageStep(1);
   bndSlider->setSliderPosition(p->thrValue);
   bndSlider->setEnabled(true);
   connect(bndSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetBndValue(int)));

   QCheckBox *binCheckBox=new QCheckBox("Binarization",this);
   binCheckBox->setGeometry(50,20,120,20);
   connect(binCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotFldBinSet(int)));

   QCheckBox *invCheckBox=new QCheckBox("Invert",this);
   invCheckBox->setGeometry(50,50,120,20);
   connect(invCheckBox,SIGNAL(stateChanged(int)),
           this,SLOT(slotFldInvSet(int)));

   setWindowTitle("Region Growing");
  }

 private slots:
  void slotSetBndValue(int x) {
   bndLabel->setText(dummyString.setNum(x)); p->floodBoundary=x;
  }
  void slotFldBinSet(int x) { p->thrBin=(bool)x; }
  void slotFldInvSet(int x) { p->thrInv=(bool)x; }

 private:
  SegMaster *p; QLabel *bndLabel; QString dummyString;
};

#endif
