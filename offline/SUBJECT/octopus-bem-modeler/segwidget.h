/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

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

#ifndef SEGWIDGET_H
#define SEGWIDGET_H

#include <QtGui>
#include "octopus_bem_master.h"
#include "sliceframe.h"

class SegWidget : public QWidget {
 Q_OBJECT
 public:
  SegWidget(QWidget *pnt,BEMMaster *sm) : QWidget(pnt) {
   int bWidth; parent=pnt; p=sm;

   sliceNoSlider=new QSlider(Qt::Vertical,this);
   sliceNoSlider->setGeometry(2,2,20,p->fHeight);
   sliceNoSlider->setRange(0,0);
   sliceNoSlider->setSingleStep(1);
   sliceNoSlider->setPageStep(1);
   sliceNoSlider->setSliderPosition(0);
   sliceNoSlider->setEnabled(false);
   connect(sliceNoSlider,SIGNAL(valueChanged(int)),
           this,SLOT(slotSetSlice(int)));

   leftFrame=new SliceFrame(this,p,24,2,"L");
   rightFrame=new SliceFrame(this,p,26+p->fWidth+62,2,"R");

   leftButtons=new QButtonGroup(); rightButtons=new QButtonGroup();
   leftButtons->setExclusive(true); rightButtons->setExclusive(true);

   QPushButton* dummyButton; bWidth=p->fWidth/p->numBuffers;
   for (int i=0;i<p->numBuffers;i++) {
    dummyButton=new QPushButton(dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(24+i*bWidth,p->fHeight+2,bWidth,18);
    leftButtons->addButton(dummyButton,i);
   }
   for (int i=0;i<p->numBuffers;i++) {
    dummyButton=new QPushButton(dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(26+62+p->fWidth+i*bWidth,p->fHeight+2,bWidth,18);
    rightButtons->addButton(dummyButton,i);
   }

   bWidth=p->fWidth/(2*p->numBounds);
   for (int i=0;i<p->numBounds;i++) {
    dummyButton=new QPushButton("M"+dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(24+i*bWidth,p->fHeight+22,bWidth,18);
    leftButtons->addButton(dummyButton,p->numBuffers+i);
   }
   for (int i=0;i<p->numBounds;i++) {
    dummyButton=new QPushButton("B"+dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(24+(p->numBounds+i)*bWidth,
                             p->fHeight+22,bWidth,18);
    leftButtons->addButton(dummyButton,p->numBuffers+p->numBounds+i);
   }
   for (int i=0;i<p->numBounds;i++) {
    dummyButton=new QPushButton("M"+dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(26+62+p->fWidth+i*bWidth,p->fHeight+22,bWidth,18);
    rightButtons->addButton(dummyButton,p->numBuffers+i);
   }
   for (int i=0;i<p->numBounds;i++) {
    dummyButton=new QPushButton("B"+dummyString.setNum(i),this);
    dummyButton-> setCheckable(true);
    dummyButton->setGeometry(26+62+p->fWidth+(p->numBounds+i)*bWidth,
                             p->fHeight+22,bWidth,18);
    rightButtons->addButton(dummyButton,p->numBuffers+p->numBounds+i);
   }

   leftButtons->button(0)->setChecked(true);
   rightButtons->button(1)->setChecked(true);

   connect(leftButtons,SIGNAL(buttonClicked(int)),
           this,SLOT(slotSelectLeft(int)));
   connect(rightButtons,SIGNAL(buttonClicked(int)),
           this,SLOT(slotSelectRight(int)));
    
   dirButton=new QPushButton("S>D",this);
   dirButton->setGeometry(26+p->fWidth,2,60,32);
   connect(dirButton,SIGNAL(clicked()),this,SLOT(slotSetDir()));

   QPushButton *copyButton=new QPushButton("COPY",this);
   copyButton->setGeometry(26+p->fWidth,52,60,18);
   connect(copyButton,SIGNAL(clicked()),this,SLOT(slotCopy()));

   QPushButton *normButton=new QPushButton("NORM",this);
   normButton->setGeometry(26+p->fWidth,52+20,60,18);
   connect(normButton,SIGNAL(clicked()),this,SLOT(slotNorm()));

   QPushButton *filtButton=new QPushButton("FILTER",this);
   filtButton->setGeometry(26+p->fWidth,52+20*2,60,18);
   connect(filtButton,SIGNAL(clicked()),this,SLOT(slotFlt()));

   QPushButton *thrButton=new QPushButton("THRESH",this);
   thrButton->setGeometry(26+p->fWidth,52+20*3,60,18);
   connect(thrButton,SIGNAL(clicked()),this,SLOT(slotThr()));

   QPushButton *edgeButton=new QPushButton("EDGES",this);
   edgeButton->setGeometry(26+p->fWidth,52+20*4,60,18);
   connect(edgeButton,SIGNAL(clicked()),this,SLOT(slotEdge()));

   QPushButton *kMeansButton=new QPushButton("KMEANS",this);
   kMeansButton->setGeometry(26+p->fWidth,52+20*5,60,18);
   connect(kMeansButton,SIGNAL(clicked()),this,SLOT(slotKMeans()));

   QPushButton *fillButton=new QPushButton("FLOOD",this);
   fillButton->setGeometry(26+p->fWidth,52+20*6,60,18);
   connect(fillButton,SIGNAL(clicked()),this,SLOT(slotFill()));

   QPushButton *setFieldButton=new QPushButton("CalcGVF",this);
   setFieldButton->setGeometry(26+p->fWidth,52+20*8,60,18);
   connect(setFieldButton,SIGNAL(clicked()),this,SLOT(slotSetField()));

   QPushButton *setSeedButton=new QPushButton("SetSeed",this);
   setSeedButton->setGeometry(26+p->fWidth,52+20*12,60,18);
   connect(setSeedButton,SIGNAL(clicked()),this,SLOT(slotSetSeed()));

   QPushButton *setNASButton=new QPushButton("SetNAS",this);
   setNASButton->setGeometry(26+p->fWidth,52+20*14,60,18);
   connect(setNASButton,SIGNAL(clicked()),this,SLOT(slotSetNAS()));

   QPushButton *setRPAButton=new QPushButton("SetRPA",this);
   setRPAButton->setGeometry(26+p->fWidth,52+20*15,60,18);
   connect(setRPAButton,SIGNAL(clicked()),this,SLOT(slotSetRPA()));

   QPushButton *setLPAButton=new QPushButton("SetLPA",this);
   setLPAButton->setGeometry(26+p->fWidth,52+20*16,60,18);
   connect(setLPAButton,SIGNAL(clicked()),this,SLOT(slotSetLPA()));

   QPushButton *clearSliceButton=new QPushButton("CLEAR",this);
   clearSliceButton->setGeometry(26+p->fWidth,52+20*18,60,18);
   connect(clearSliceButton,SIGNAL(clicked()),this,SLOT(slotClearSlice()));

   QPushButton *brush0Button=new QPushButton("BRUSH0",this);
   brush0Button->setGeometry(26+p->fWidth,52+20*20,60,18);
   connect(brush0Button,SIGNAL(clicked()),this,SLOT(slotSetBrush0()));

   QPushButton *brush1Button=new QPushButton("BRUSH1",this);
   brush1Button->setGeometry(26+p->fWidth,52+20*21,60,18);
   connect(brush1Button,SIGNAL(clicked()),this,SLOT(slotSetBrush1()));

   QPushButton *brush2Button=new QPushButton("BRUSH2",this);
   brush2Button->setGeometry(26+p->fWidth,52+20*22,60,18);
   connect(brush2Button,SIGNAL(clicked()),this,SLOT(slotSetBrush2()));

   QPushButton *brush3Button=new QPushButton("BRUSH3",this);
   brush3Button->setGeometry(26+p->fWidth,52+20*23,60,18);
   connect(brush3Button,SIGNAL(clicked()),this,SLOT(slotSetBrush3()));

   QPushButton *scalpMaskButton=new QPushButton("ScalpM",this);
   scalpMaskButton->setGeometry(26+p->fWidth,52+20*25,60,18);
   connect(scalpMaskButton,SIGNAL(clicked()),this,SLOT(slotScalpMask()));
   QPushButton *skullMaskButton=new QPushButton("SkullM",this);
   skullMaskButton->setGeometry(26+p->fWidth,52+20*26,60,18);
   connect(skullMaskButton,SIGNAL(clicked()),this,SLOT(slotSkullMask()));
   QPushButton *brainMaskButton=new QPushButton("BrainM",this);
   brainMaskButton->setGeometry(26+p->fWidth,52+20*27,60,18);
   connect(brainMaskButton,SIGNAL(clicked()),this,SLOT(slotBrainMask()));
   QPushButton *scalpBoundButton=new QPushButton("ScalpB",this);
   scalpBoundButton->setGeometry(26+p->fWidth,52+20*28,60,18);
   connect(scalpBoundButton,SIGNAL(clicked()),this,SLOT(slotScalpBound()));
   QPushButton *skullBoundButton=new QPushButton("SkullB",this);
   skullBoundButton->setGeometry(26+p->fWidth,52+20*29,60,18);
   connect(skullBoundButton,SIGNAL(clicked()),this,SLOT(slotSkullBound()));
   QPushButton *brainBoundButton=new QPushButton("BrainB",this);
   brainBoundButton->setGeometry(26+p->fWidth,52+20*30,60,18);
   connect(brainBoundButton,SIGNAL(clicked()),this,SLOT(slotBrainBound()));

   QPushButton *previewButton=new QPushButton("PREV",this);
   previewButton->setGeometry(26+p->fWidth,p->fHeight-32,60,32);
   previewButton->setCheckable(true);
   previewButton->setDown(p->previewMode);
   connect(previewButton,SIGNAL(toggled(bool)),
           this,SLOT(slotSetPreviewMode(bool)));
  }

  void loadUpdate() {
   if (p->mrSetExists) {
    sliceNoSlider->setRange(0,p->zSize-1);
    sliceNoSlider->setSliderPosition(1); sliceNoSlider->setEnabled(true);
    sliceNoSlider->setValue(p->zSize-1);
   } else {
    sliceNoSlider->setRange(0,0);
    sliceNoSlider->setSliderPosition(0); sliceNoSlider->setEnabled(false);
   }
   leftFrame->repaint(); rightFrame->repaint();
  }

  void processUpdate() { leftFrame->repaint(); rightFrame->repaint(); }

  void preview() {
   if (dirButton->text()=="S>D") leftFrame->repaint();
   else if (dirButton->text()=="D<S") rightFrame->repaint();
  }

 public slots:
  void slotSetSlice(int s) {
   sliceNoSlider->setValue(s); p->curSlice=s;
   leftFrame->repaint(); rightFrame->repaint();
  }

  void slotSetDir() { int dummy;
   if (dirButton->text()=="S>D") {
    dirButton->setText("D<S"); p->curFrame=1;
   } else if (dirButton->text()=="D<S") {
    dirButton->setText("S>D"); p->curFrame=0;
   } dummy=p->srcX; p->srcX=p->dstX; p->dstX=dummy;
  }

  void slotSelectLeft(int v)  { p->curBufferL=v;
   if (dirButton->text()=="S>D") {
    p->srcX=p->curBufferL; p->dstX=p->curBufferR;
   } else if (dirButton->text()=="D<S") {
    p->dstX=p->curBufferL; p->srcX=p->curBufferR;
   } leftFrame->repaint(); rightFrame->repaint();
  }
  void slotSelectRight(int v) { p->curBufferR=v;
   if (dirButton->text()=="S>D") {
    p->srcX=p->curBufferL; p->dstX=p->curBufferR;
   } else if (dirButton->text()=="D<S") {
    p->dstX=p->curBufferL; p->srcX=p->curBufferR;
   } leftFrame->repaint(); rightFrame->repaint();
  }

  void slotSetPreviewMode(bool x) {
   if (x) { p->previewMode=true; }
   else { p->previewMode=false;
    leftFrame->repaint(); rightFrame->repaint();
   }
  }

  void slotSetBrush0() { p->brushSize= 1; }
  void slotSetBrush1() { p->brushSize= 3; }
  void slotSetBrush2() { p->brushSize=10; }
  void slotSetBrush3() { p->brushSize=25; }

  void slotCopy()     { p->processCopyStart();       }
  void slotNorm()     { p->processNormStart();       }
  void slotFlt()      { p->processFltStart();        }
  void slotThr()      { p->processThrStart();        }
  void slotEdge()     { p->processEdgeStart();       }
  void slotKMeans()   { p->processKMStart();         }
  void slotFill()     { p->processFillStart();       }
  void slotSetField() { p->processComputeGVFField(); }

  void slotSetSeed()    { p->setSeed(); }
  void slotSetNAS()     { p->setNAS(); }
  void slotSetRPA()     { p->setRPA(); }
  void slotSetLPA()     { p->setLPA(); }
  void slotClearSlice() { p->clearSlice(); }

  void slotScalpMask()  { p->copyBuffer(p->srcX,p->numBuffers+0); }
  void slotScalpBound() { p->copyBuffer(p->srcX,p->numBuffers+p->numBounds+0); }

  void slotSkullMask()  { p->copyBufferMasked(p->srcX,&(p->mask[0]),
                                              p->numBuffers+1); }
  void slotBrainMask()  { p->copyBufferMasked(p->srcX,&(p->mask[1]),
                                              p->numBuffers+2); }
  void slotSkullBound() { p->copyBufferMasked(p->srcX,&(p->mask[1]),
                                              p->numBuffers+p->numBounds+1); }
  void slotBrainBound() { p->copyBufferMasked(p->srcX,&(p->mask[2]),
                                              p->numBuffers+p->numBounds+2); }

 private:
  QWidget *parent; BEMMaster *p;
  QSlider *sliceNoSlider; //QButtonGroup *leftSelGroup,*rightSelGroup;
  SliceFrame *leftFrame,*rightFrame; QString dummyString;
  QButtonGroup *leftButtons,*rightButtons; QPushButton *dirButton;
};

#endif
