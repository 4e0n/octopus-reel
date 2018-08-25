/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef SEGWIDGET_H
#define SEGWIDGET_H

#include <QtGui>
#include "octopus_seg_master.h"
#include "sliceframe.h"

class SegWidget : public QWidget {
 Q_OBJECT
 public:
  SegWidget(QWidget *pnt,SegMaster *sm) : QWidget(pnt) {
   parent=pnt; p=sm;

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
   rightFrame=new SliceFrame(this,p,26+p->fWidth+22,2,"R");

   leftButtons=new QButtonGroup(); rightButtons=new QButtonGroup();
   leftButtons->setExclusive(true); rightButtons->setExclusive(true);

   QPushButton* dummyButton; int bWidth=p->fWidth/p->numBuffers;
   for (int i=0;i<p->numBuffers;i++) {
    dummyButton=new QPushButton(dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(24+i*bWidth,p->fHeight+2,bWidth,18);
    leftButtons->addButton(dummyButton,i);
   }
   for (int i=0;i<p->numBuffers;i++) {
    dummyButton=new QPushButton(dummyString.setNum(i),this);
    dummyButton->setCheckable(true);
    dummyButton->setGeometry(26+22+p->fWidth+i*bWidth,p->fHeight+2,bWidth,18);
    rightButtons->addButton(dummyButton,i);
   }
   leftButtons->button(0)->setChecked(true);
   rightButtons->button(1)->setChecked(true);

   connect(leftButtons,SIGNAL(buttonClicked(int)),
           this,SLOT(slotSelectLeft(int)));
   connect(rightButtons,SIGNAL(buttonClicked(int)),
           this,SLOT(slotSelectRight(int)));
    
   dirButton=new QPushButton(">",this);
   dirButton->setGeometry(26+p->fWidth,2,20,32);
   connect(dirButton,SIGNAL(clicked()),this,SLOT(slotSetDir()));

   QPushButton *copyButton=new QPushButton("C",this);
   copyButton->setGeometry(26+p->fWidth,42,20,24);
   connect(copyButton,SIGNAL(clicked()),p,SLOT(slotCopyStart()));

   QPushButton *normButton=new QPushButton("N",this);
   normButton->setGeometry(26+p->fWidth,42+28,20,24);
   connect(normButton,SIGNAL(clicked()),p,SLOT(slotNormStart()));

   QPushButton *filtButton=new QPushButton("F",this);
   filtButton->setGeometry(26+p->fWidth,42+28*2,20,24);
   connect(filtButton,SIGNAL(clicked()),p,SLOT(slotFltStart()));

   QPushButton *thrButton=new QPushButton("T",this);
   thrButton->setGeometry(26+p->fWidth,42+28*3,20,24);
   connect(thrButton,SIGNAL(clicked()),p,SLOT(slotThrStart()));

   QPushButton *edgeButton=new QPushButton("E",this);
   edgeButton->setGeometry(26+p->fWidth,42+28*4,20,24);
   connect(edgeButton,SIGNAL(clicked()),p,SLOT(slotEdgeStart()));

   QPushButton *kMeansButton=new QPushButton("K",this);
   kMeansButton->setGeometry(26+p->fWidth,42+28*5,20,24);
   connect(kMeansButton,SIGNAL(clicked()),p,SLOT(slotKMeansStart()));

   QPushButton *srf1Button=new QPushButton("Sc",this);
   srf1Button->setGeometry(26+p->fWidth,194+28,20,24);
   connect(srf1Button,SIGNAL(clicked()),p,SLOT(slotSrf1Start()));

   QPushButton *srf2Button=new QPushButton("Sk",this);
   srf2Button->setGeometry(26+p->fWidth,194+28*2,20,24);
   connect(srf2Button,SIGNAL(clicked()),p,SLOT(slotSrf2Start()));

   QPushButton *srf3Button=new QPushButton("Br",this);
   srf3Button->setGeometry(26+p->fWidth,194+28*3,20,24);
   connect(srf3Button,SIGNAL(clicked()),p,SLOT(slotSrf3Start()));

   QPushButton *srf4Button=new QPushButton("4",this);
   srf4Button->setGeometry(26+p->fWidth,194+28*4,20,24);
   connect(srf4Button,SIGNAL(clicked()),p,SLOT(slotSrf4Start()));

   QPushButton *selP1Button=new QPushButton("A",this);
   selP1Button->setGeometry(26+p->fWidth,340+28*0,20,24);
   connect(selP1Button,SIGNAL(clicked()),p,SLOT(slotSelP1()));

   QPushButton *selP2Button=new QPushButton("L",this);
   selP2Button->setGeometry(26+p->fWidth,340+28*1,20,24);
   connect(selP2Button,SIGNAL(clicked()),p,SLOT(slotSelP2()));

   QPushButton *selP3Button=new QPushButton("R",this);
   selP3Button->setGeometry(26+p->fWidth,340+28*2,20,24);
   connect(selP3Button,SIGNAL(clicked()),p,SLOT(slotSelP3()));

   QPushButton *clrButton=new QPushButton("X",this);
   clrButton->setGeometry(26+p->fWidth,430,20,24);
   connect(clrButton,SIGNAL(clicked()),p,SLOT(slotClrSlice()));

   QPushButton *brush0Button=new QPushButton("0",this);
   brush0Button->setGeometry(26+p->fWidth,430+28*1,20,24);
   connect(brush0Button,SIGNAL(clicked()),this,SLOT(slotSetBrush0()));

   QPushButton *brush1Button=new QPushButton("1",this);
   brush1Button->setGeometry(26+p->fWidth,430+28*2,20,24);
   connect(brush1Button,SIGNAL(clicked()),this,SLOT(slotSetBrush1()));

   QPushButton *brush2Button=new QPushButton("2",this);
   brush2Button->setGeometry(26+p->fWidth,430+28*3,20,24);
   connect(brush2Button,SIGNAL(clicked()),this,SLOT(slotSetBrush2()));

   QPushButton *previewButton=new QPushButton("P",this);
   previewButton->setGeometry(26+p->fWidth,p->fHeight-32,20,32);
   previewButton->setCheckable(true);
   previewButton->setDown(p->previewMode);
   connect(previewButton,SIGNAL(toggled(bool)),
           this,SLOT(slotSetPreviewMode(bool)));
  }

  void loadUpdate() {
   if (p->zSize) {
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
   if (dirButton->text()==">") leftFrame->repaint();
   else if (dirButton->text()=="<") rightFrame->repaint();
  }

 public slots:
  void slotSetSlice(int s) {
   sliceNoSlider->setValue(s); p->curSlice=s;
   leftFrame->repaint(); rightFrame->repaint();
  }

  void slotSetDir() { int dummy;
   if (dirButton->text()==">") { dirButton->setText("<"); p->curFrame=1; }
   else if (dirButton->text()=="<") { dirButton->setText(">"); p->curFrame=0; }
   dummy=p->srcX; p->srcX=p->dstX; p->dstX=dummy;
  }

  void slotSelectLeft(int v)  { p->curVolumeL=v;
   if (dirButton->text()==">") {
    p->srcX=p->curVolumeL; p->dstX=p->curVolumeR;
   } else if (dirButton->text()=="<") {
    p->dstX=p->curVolumeL; p->srcX=p->curVolumeR;
   } leftFrame->repaint(); rightFrame->repaint();
  }
  void slotSelectRight(int v) { p->curVolumeR=v;
   if (dirButton->text()==">") {
    p->srcX=p->curVolumeL; p->dstX=p->curVolumeR;
   } else if (dirButton->text()=="<") {
    p->dstX=p->curVolumeL; p->srcX=p->curVolumeR;
   } leftFrame->repaint(); rightFrame->repaint();
  }

  void slotSetPreviewMode(bool x) {
   if (x) { p->previewMode=true; }
   else { p->previewMode=false;
    leftFrame->repaint(); rightFrame->repaint();
   }
  }

  void slotSetBrush0() { p->brushSize=10; }
  void slotSetBrush1() { p->brushSize=25; }
  void slotSetBrush2() { p->brushSize=75; }

 private:
  QWidget *parent; SegMaster *p;
  QSlider *sliceNoSlider; //QButtonGroup *leftSelGroup,*rightSelGroup;
  SliceFrame *leftFrame,*rightFrame; QString dummyString;
  QButtonGroup *leftButtons,*rightButtons; QPushButton *dirButton;
};

#endif
