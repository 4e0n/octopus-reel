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

#ifndef ACQSTREAMWINDOW_H
#define ACQSTREAMWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QFrame>
#include <QMenu>
#include <QMenuBar>
#include <QSlider>
#include <QCheckBox>
#include <QListWidget>
//#include <QAbstractButton>
//#include <QMessageBox>
//#include <QListWidgetItem>
#include "confparam.h"
#include "eegframe.h"

class AcqStreamWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit AcqStreamWindow(unsigned int a=0,ConfParam *c=nullptr,QWidget *parent=nullptr) : QMainWindow(parent) {
   ampNo=a; conf=c;
   setGeometry(ampNo*conf->guiStrmW+conf->guiStrmX,conf->guiStrmY,conf->guiStrmW,conf->guiStrmH);
   setFixedSize(conf->guiStrmW,conf->guiStrmH);

   conf->eegFrameH=conf->guiStrmH-90; conf->eegFrameW=(int)(.66*(float)(conf->guiStrmW));
   if (conf->eegFrameW%2==1) conf->eegFrameW--;
   conf->glFrameW=(int)(.33*(float)(conf->guiStrmW));
   if (conf->glFrameW%2==1) { conf->glFrameW--; } conf->glFrameH=conf->glFrameW;

   // *** TABS & TABWIDGETS ***

   mainTabWidget=new QTabWidget(this);
   mainTabWidget->setGeometry(1,32,this->width()-2,this->height());

   eegWidget=new QWidget(mainTabWidget);
   eegFrame=new EEGFrame(conf,ampNo,eegWidget);
   eegFrame->setGeometry(2,2,conf->eegFrameW,conf->eegFrameH); eegWidget->show(); 
 
   mainTabWidget->addTab(eegWidget,"EEG/ERP"); mainTabWidget->show();

   // *** EEG & ERP VISUALIZATION BUTTONS AT THE BOTTOM ***

   eegAmpBG=new QButtonGroup(); eegAmpBG->setExclusive(true);
   for (int i=0;i<6;i++) { // EEG Amplification
    dummyButton=new QPushButton(eegWidget); dummyButton->setCheckable(true);
    dummyButton->setGeometry(100+i*60,eegWidget->height(),60,20);
    eegAmpBG->addButton(dummyButton,i);
   }
   eegAmpBG->button(0)->setText("1mV");   eegAmpBG->button(1)->setText("500uV");
   eegAmpBG->button(2)->setText("200uV"); eegAmpBG->button(3)->setText("100uV");
   eegAmpBG->button(4)->setText("50uV");  eegAmpBG->button(5)->setText("20uV");
   eegAmpBG->button(3)->setDown(true);
   conf->eegAmpX[ampNo]=(1000000.0/100.0);
   connect(eegAmpBG,SIGNAL(buttonClicked(int)),this,SLOT(slotEEGAmp(int)));

   // *** MENUBAR ***

   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *modelMenu=new QMenu("&Model",menuBar); QMenu *viewMenu=new QMenu("&View",menuBar);
   menuBar->addMenu(modelMenu); menuBar->addMenu(viewMenu);
   setMenuBar(menuBar);

   setWindowTitle("Octopus Hyper EEG/ERP Amp #"+QString::number(ampNo+1));
  }

  ~AcqStreamWindow() override {}
/*
  //void scroll(std::vector<float> *data,std::vector<float> *dataF) {//QVector<float> *m,QVector<float> *s) {
  void scroll(QVector<float> *mean,QVector<float> *std) {
   //QVector<float> means,stdevs;
   //computeMeanAndStdev(means,stdevs);
   //qDebug() << "Data:" << (*data)[0] << "DataF:" << (*dataF)[0];
   //qDebug() << "Means[0]:" << means[0] << "Stdev[0]:" << stdevs[0];
   eegFrame->scroll(false,false,mean,std);
  }
*/
//  void updateEEG(const TcpSample &smp) {
//   qDebug("here");
   //for (int amp = 0; amp < smp.amp.size(); ++amp) {
   // if (!smp.amp[amp].dataF.empty()) {
   //  eegFrame->pushSample(amp, smp.amp[amp].dataF[0]);  // filtered sample
   // }
   //}
   //eegFrame->refresh();
//  }

/*
void updateEEG(const TcpSample &smp) {
    QVector<float> means;

    for (int amp = 0; amp < smp.amp.size(); ++amp) {
        if (!smp.amp[amp].dataF.empty()) {
            means.append(smp.amp[amp].dataF[0]);  // Use filtered value (e.g., first channel)
        }
    }

    eegFrame->updateEEG(means);
}
*/
/*  void updateEEG(const TcpSample &tcpSample) {
   //qDebug() << "[AcqStreamWindow@updateEEG] amp=0 raw=" << tcpSample.amp[0].data[0]
   //         << " flt=" << tcpSample.amp[0].dataF[0];

   if (ampNo>=tcpSample.amp.size()) return;
   const Sample &s=tcpSample.amp[ampNo];
   int nChn=s.dataF.size();
   // Initialize rolling buffer if needed
   if (rollingWindow.size()!=nChn) rollingWindow.resize(nChn);
   // Push new sample for each channel
   for (int ch=0;ch<nChn;++ch) {
    rollingWindow[ch].append(s.dataF[ch]);
    //qDebug() << "AmpIdx:" << ampNo << "Ch:" << ch << "Val:" << s.dataF[ch] << "BufferSize:" << rollingWindow[ch].size();
    //qDebug() << rollingWindow[ch].size();
    if (rollingWindow[ch].size()>rollingWindowSize) rollingWindow[ch].removeFirst();
   }
   //if (ampNo < static_cast<unsigned int>(tcpSample.amp.size())) {
   // eegFrame->pushSample(tcpSample.amp[ampNo]);
   // eegFrame->update();
   //}

    eegFrame->updateEEG(tcpSample);

  }
*/
  ConfParam *conf;
  
  QVector<QVector<float>> rollingWindow; // [channel][time]
  //int rollingWindowSize; // e.g., 100 samples for 100 ms window
  unsigned int ampNo;
 
 public slots:
  void slotEEGAmp(int x) {
   switch (x) {
    case 0: conf->eegAmpX[ampNo]=(1000000.0/1000.0);eegAmpBG->button(3)->setDown(false);break;
    case 1: conf->eegAmpX[ampNo]=(1000000.0/500.0);eegAmpBG->button(3)->setDown(false);break;
    case 2: conf->eegAmpX[ampNo]=(1000000.0/200.0);eegAmpBG->button(3)->setDown(false);break;
    case 3: conf->eegAmpX[ampNo]=(1000000.0/100.0);break;
    case 4: conf->eegAmpX[ampNo]=(1000000.0/50.0);eegAmpBG->button(3)->setDown(false);break;
    case 5: conf->eegAmpX[ampNo]=(1000000.0/20.0);eegAmpBG->button(3)->setDown(false);break;
   }
  }

 private:
  QString dummyString; QPushButton *dummyButton;
  EEGFrame *eegFrame;
  QMenuBar *menuBar; QTabWidget *mainTabWidget; QWidget *eegWidget;
  QButtonGroup *eegAmpBG;
  QVector<QPushButton*> eegAmpButtons;
};

#endif

