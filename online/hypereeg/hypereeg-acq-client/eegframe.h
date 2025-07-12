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

#ifndef EEGFRAME_H
#define EEGFRAME_H

//#include <QWidget>
#include <QFrame>
//#include <QImage>
#include <QPainter>
#include <QBitmap>
//#include <QImage>
#include <QMatrix>
#include <QStaticText>
#include <cmath>
#include <QVector>
//#include <QMap>
//#include <QDebug>
//#include <QtMath>
//#include "../tcpsample.h"
#include "confparam.h"

// Simple structure to hold mean and standard deviation for each time point
//struct EEGScrollPoint { float mean=0.0f; float stdev=0.0f; };

class EEGFrame : public QFrame { // QWidget {
 Q_OBJECT
 public:
  //explicit EEGFrame(QWidget *parent=nullptr) : QWidget(parent),scrollImage(1000,600,QImage::Format_RGB32) {
  //explicit EEGFrame(unsigned int a=0,QWidget *parent = nullptr) : QFrame(parent),scrollImage(800, 600, QImage::Format_RGB32) {
  // scrollImage.fill(Qt::black);
  explicit EEGFrame(ConfParam *c=nullptr,unsigned int a=0,QWidget *parent=nullptr) : QFrame(parent) {
   conf=c; ampNo=a; scrollSched=false;

   chnCount=conf->chns.size();
   colCount=std::ceil((float)chnCount/(float)(33.));
   chnPerCol=std::ceil((float)(chnCount)/(float)(colCount));
   int ww=(int)((float)(conf->eegFrameW)/(float)colCount);
   for (int i=0;i<colCount;i++) { w0.append(i*ww+1); wX.append(i*ww+1); }
   for (int i=0;i<colCount-1;i++) wn.append((i+1)*ww-1);
   wn.append(conf->eegFrameW-1);

   if (chnCount<16) chnFont=QFont("Helvetica",12,QFont::Bold);
   else if (chnCount>16 && chnCount<32) chnFont=QFont("Helvetica",11,QFont::Bold);
   else if (chnCount>96) chnFont=QFont("Helvetica",10);

   evtFont=QFont("Helvetica",14,QFont::Bold);
   rTransform.rotate(-90);

   rBuffer=QPixmap(120,24); rBuffer.fill(Qt::transparent);
   rBufferC=QPixmap(); // cached - empty until first use

   scrollBuffer=QPixmap(conf->eegFrameW,conf->eegFrameH); resetScrollBuffer();

   chnTextCache.clear(); chnTextCache.reserve(chnCount);

   for (int i=0;i<chnCount;++i) {
    const QString& chName=conf->chns[i].chnName;
    int chNo=conf->chns[i].physChn;

    QString label=QString::number(chNo)+" "+chName;
    QStaticText staticLabel(label); staticLabel.setTextFormat(Qt::PlainText);
    staticLabel.setTextWidth(-1);  // No width constraint
    chnTextCache.append(staticLabel);
   }

   scrCurData.resize(chnCount); scrCurData.fill(0.0f,chnCount);
   scrCurDataF.resize(chnCount); scrCurDataF.fill(0.0f,chnCount);
   scrPrvData.resize(chnCount); scrPrvData.fill(0.0f,chnCount);
   scrPrvDataF.resize(chnCount); scrPrvDataF.fill(0.0f,chnCount);
  }

  void scroll(bool t,bool e,QVector<float> *mean,QVector<float> *std) {
   scrollSched=true; tick=t; event=e;
   for (int i=0;i<chnCount;i++) {
    scrPrvData[i]=scrCurData[i]; scrPrvDataF[i]=scrCurDataF[i]; // Backup prev
    // fetch computed data to current
    scrCurData[i]=(int)((*mean)[i]*100000.);
    scrCurDataF[i]=(int)((*std)[i]*100000.);
   }
   updateBuffer(); update();
  }

  void resetScrollBuffer() {
   QPainter scrollPainter;
   QRect cr(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
   scrollBuffer.fill(Qt::white);
   scrollPainter.begin(&scrollBuffer);
    scrollPainter.setPen(Qt::red);
    scrollPainter.drawRect(cr);
    scrollPainter.drawLine(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
   scrollPainter.end();
  }

//  void pushSample(const Sample &sample) {
//   int nChn=static_cast<int>(sample.dataF.size());
//   // Ensure our internal buffers are sized correctly
//   if (scrCurData.size() != nChn) {
//    scrCurData.resize(nChn); scrCurData.fill(0.0f,nChn);
//    scrPrvData.resize(nChn); scrPrvData.fill(0.0f,nChn);
//   }
//   // Shift previous values and store new ones
//   for (int ch=0;ch<nChn;++ch) { scrPrvData[ch]=scrCurData[ch]; scrCurData[ch]=sample.dataF[ch]; } // Using filtered data
//   updateBuffer();
//  }

  void updateBuffer() {
   QPainter scrollPainter; int curCol;
   scrollPainter.begin(&scrollBuffer);
    scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Cursor vertical line for each column
    for (c=0;c<colCount;c++) {
     if (wX[c]<=wn[c]) {
      scrollPainter.setPen(Qt::white); scrollPainter.drawLine(wX[c],1,wX[c],conf->eegFrameH-2);
      if (wX[c]<(wn[c]-1)) { scrollPainter.setPen(Qt::black); scrollPainter.drawLine(wX[c]+1,1,wX[c]+1,conf->eegFrameH-2); }
     }
    }

    //if (tick) {
    // scrollPainter.setPen(Qt::darkGray); scrollPainter.drawRect(0,0,conf->eegFrameW-1,conf->eegFrameH-1);
    // for (c=0;c<colCount;c++) {
    //  scrollPainter.drawLine(w0[c]-1,1,w0[c]-1,conf->eegFrameH-2);
    //  scrollPainter.drawLine(wX[c],1,wX[c],conf->eegFrameH-2);
    // }
    //}

    for (int i=0;i<chnCount;i++) {
     c=chnCount/chnPerCol; curCol=i/chnPerCol;
     chHeight=100.0; // 100 pixel is the indicated amp..
     chY=(float)(conf->eegFrameH)/(float)(chnPerCol); scrCurY=(int)(chY/2.0+chY*(i%chnPerCol));


     //scrPrvData[i]=scrCurData[i]; scrPrvDataF[i]=scrCurDataF[i]; // Also backup previous
     //scrCurData[i]=
     //scrCurDataF[i]=
     //scrPrvDataX=scrPrvData[i]; scrCurDataX=scrCurData[i];
     //scrPrvDataFX=scrPrvDataF[i]; scrCurDataFX=scrCurDataF[i];

     // Baseline
     scrollPainter.setPen(Qt::darkGray);
     //scrollPainter.drawLine(wX[curCol]-1,scrCurY,wX[curCol],scrCurY);

     scrollPainter.setPen(Qt::black);
     scrollPainter.drawLine( // Div400 bcs. range is 400uVpp..
      wX[curCol]-1,
      scrCurY-(int)(scrPrvData[i]), //*chHeight*conf->eegAmpX[ampNo]/4.0),
      wX[curCol],
      scrCurY-(int)(scrCurData[i]) //*chHeight*conf->eegAmpX[ampNo]/4.0)
     );
    }

//   if (event) { event=false; rBuffer.fill(Qt::transparent);
//    rotPainter.begin(&rBuffer);
//     rotPainter.setRenderHint(QPainter::TextAntialiasing,true);
//     rotPainter.setFont(evtFont); QColor penColor=Qt::darkGreen;
//     if (conf->curEventType==1) penColor=Qt::blue; else if (conf->curEventType==2) penColor=Qt::red;
//     rotPainter.setPen(penColor); rotPainter.drawText(2,9,conf->curEventName);
//    rotPainter.end();
//    rBufferC=rBuffer.transformed(rTransform,Qt::SmoothTransformation);
//
//    //scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
//    //for (c=0;c<chnCount/chnPerCol;c++) {
//    // scrollPainter.drawLine(wX[c]-1,1,wX[c]-1,conf->eegFrameH-1);
//    // scrollPainter.drawPixmap(wX[c]-15,conf->eegFrameH-104,rBufferC);
//    //}
//    
//    scrollPainter.setCompositionMode(QPainter::CompositionMode_SourceOver); scrollPainter.setPen(Qt::blue); // Line color
//    QVector<QPainter::PixmapFragment> fragments; fragments.reserve(chnCount / chnPerCol);
//    for (c=0;c<chnCount/chnPerCol;c++) { int lineX=wX[c]-1;
//     scrollPainter.drawLine(lineX,1,lineX,conf->eegFrameH-1);
//     // Prepare batched label blitting - for source in pixmap
//     fragments.append(QPainter::PixmapFragment::create(QPointF(lineX-14,conf->eegFrameH-104),
//                                                       QRectF(0,0,rBufferC.width(),rBufferC.height())));
//    }
//    scrollPainter.drawPixmapFragments(fragments.constData(),fragments.size(),rBufferC);
//   }

   // Channel names
   scrollPainter.setPen(QColor(50,50,150)); scrollPainter.setFont(chnFont);
   for (int i=0;i<chnCount;++i) { curCol=i/chnPerCol;
    chY=(float)(conf->eegFrameH)/(float)(chnPerCol); scrCurY=(int)(5+chY/2.0+chY*(i%chnPerCol));
    scrollPainter.drawStaticText(w0[curCol]+4,scrCurY,chnTextCache[i]);
   }

//   if (wX[0]==150) scrollPainter.drawLine(149,300,149,400); // Amplitude legend

   // Main position increments of columns
   for (c=0;c<colCount;c++) { wX[c]++; if (wX[c]>wn[c]) wX[c]=w0[c]; }

   scrollPainter.end();
  }

 public slots:
  void slotScrData(bool t,bool e) { scrollSched=true; tick=t; event=e; updateBuffer(); repaint(); }

 protected:
  virtual void paintEvent(QPaintEvent *event) override {
   Q_UNUSED(event);
   mainPainter.begin(this);
   mainPainter.drawPixmap(0,0,scrollBuffer); scrollSched=false;
   mainPainter.end();
  }

 private:
  ConfParam *conf; unsigned int ampNo; QString dummyString;
  QBrush bgBrush; QPainter mainPainter,rotPainter; QVector<int> w0,wn,wX;
  QFont evtFont,chnFont; QPixmap rBuffer,rBufferC; QTransform rTransform;
  float chHeight,chY; int scrCurY;
  bool scrollSched,tick,event; int c,chnCount,colCount,chnPerCol; QPixmap scrollBuffer;
  QVector<QStaticText> chnTextCache;
  QVector<float> scrCurData,scrPrvData,scrCurDataF,scrPrvDataF;
};

#endif

//   QImage temp = scrollImage.copy(1, 0, scrollImage.width() - 1, scrollImage.height());
//   scrollImage.fill(Qt::black);
//   QPainter fbPainter(&scrollImage);
//   fbPainter.drawImage(0, 0, temp);
//   int chHeight = scrollImage.height() / qMax(1, channels.size());
//   float scaleY = 100.0f;  // Adjust for amplitude
//   for (int ch = 0; ch < channels.size(); ++ch) {
//    int yMid = ch * chHeight + chHeight / 2;
//    float value = channels[ch].isEmpty() ? 0.0f : channels[ch].last();
//    int y = yMid - static_cast<int>(value * scaleY);
//    fbPainter.setPen(Qt::green);
//    fbPainter.drawPoint(scrollImage.width() - 2, y);
//   }
//   QPainter widgetPainter(this);
//   widgetPainter.drawImage(0, 0, scrollImage);

//private:
//  unsigned int ampNo;
//    QImage scrollImage;
//    QVector<QVector<float>> channels;
//    const int maxPoints = 500;

//  // Push one set of mean+stdev for all channels at the current timepoint
//  void pushSample(const QVector<float> &channelMeans,const QVector<float> &channelStdevs) {
//  // qDebug() << "[pushSample] Means[0]=" << channelMeans[0] << " Stdev[0]=" << channelStdevs[0];
//   int nChn=channelMeans.size();
//   if (scrollPoints.size()!=nChn) {
//    scrollPoints.resize(nChn);
//    for (auto &vec:scrollPoints) vec.resize(maxWidth);
//    // For each channel, update scroll position and write new point
//    int curCol;
//    for (int ch=0;ch<nChn;++ch) {
//     curCol=ch/maxChnPerCol();
//     int colScrollX=wX.value(curCol,0);
//     // Explicit assignment instead of brace initializer
//     EEGScrollPoint pt;
//     pt.mean=channelMeans[ch];
//     pt.stdev=channelStdevs[ch];
//     scrollPoints[ch][colScrollX]=pt;
//     // Brace Initializer -- more elegant C++11 but didn't compile.
//     //curCol=ch/maxChnPerCol();
//     //int colScrollX=wX.value(curCol,0);
//     //scrollPoints[ch][colScrollX]={channelMeans[ch],channelStdevs[ch]};
//    }
//    // Advance scroll positions for each column
//    int totalCols=(nChn+maxChnPerCol()-1)/maxChnPerCol();
//    for (int c=0;c<totalCols;++c) wX[c]=(wX.value(c,0)+1)%maxWidth;
//   }
//   update();  // Trigger paint
//  }

//  void updateEEG(const TcpSample &sample) {
//   for (int ch = 0; ch < chnCount; ++ch) {
//    float mean = sample.amp[0].data[ch];  // or dataF[ch] if filtered
//    int yOffset = static_cast<int>(mean * scaleY);
//    chnValues[ch][scrollX] = yOffset;
//   }
//   scrollX = (scrollX + 1) % maxPoints;
//   update();
//  }

//   qDebug("PaintEvent");
//   QPainter p(this);
//   QPainter fbPainter(&scrollImage);
//   fbPainter.fillRect(scrollImage.rect(),Qt::black);
//   int nChn=scrollPoints.size();
//   if (nChn==0) return;
//   int colCount=(nChn+maxChnPerCol()-1)/maxChnPerCol();
//   int colWidth=scrollImage.width()/colCount;
//   int rowHeight=scrollImage.height()/maxChnPerCol();
//   for (int ch=0;ch<nChn;++ch) {
//    int curCol=ch/maxChnPerCol();
//    int colX=curCol*colWidth;
//    int xPos=colX+wX.value(curCol,0);
//    int row=ch%maxChnPerCol();
//    int yMid=row*rowHeight+rowHeight/2;
//    const auto &buffer=scrollPoints[ch];
//    int idx=wX.value(curCol,0);
//    const EEGScrollPoint &pt=buffer[idx];
//    float scaleY=30.0f;  // Pixels per microvolt, adjustable
//    int yMean=yMid-static_cast<int>(pt.mean*scaleY);
//    int yUpper=yMean-static_cast<int>(pt.stdev*scaleY);
//    int yLower=yMean+static_cast<int>(pt.stdev*scaleY);
//    // Draw standard deviation aura as vertical line
//    fbPainter.setPen(QColor(0,255,0,80));
//    //fbPainter.drawLine(xPos,yUpper,xPos,yLower);
//    // Draw mean point
//    fbPainter.setPen(Qt::green);
//    fbPainter.drawPoint(xPos,yMean);
//   }
//   p.drawImage(0,0,scrollImage);
//  }

//    // Scroll left by 1 pixel
//    QImage temp = scrollImage.copy(1, 0, scrollImage.width() - 1, scrollImage.height());
//    scrollImage.fill(Qt::black);  // Clear old image
//    QPainter fbPainter(&scrollImage);
//    fbPainter.drawImage(0, 0, temp);

//    for (int ch = 0; ch < chnCount; ++ch) {
//     int baselineY = ch * (height() / chnCount) + (height() / chnCount) / 2;
//     int yVal = baselineY - chnValues[ch][scrollX];
//     fbPainter.drawPoint(scrollImage.width() - 1, yVal);
//    }

//    p.drawImage(0, 0, scrollImage);

//   private:
//    QVector<QVector<EEGScrollPoint>> scrollPoints; // [channel][timeIndex]
//    QMap<int,int> wX; // Per-column scroll positions
//    const int maxWidth=1000; // Horizontal scroll size in pixels
//    int maxChnPerCol() const { return 33; } // Adjustable max channels per visual column
//    QImage scrollImage;
//    QVector<QVector<int>> chnValues;  // [channel][scrollX] → value (Y displacement)
//    int scrollX = 0;
//    int maxPoints = 1000;  // width in pixels
//    int chnCount = 8;      // example: 8 channels
//    float scaleY = 30.0f;  // pixels per μV
