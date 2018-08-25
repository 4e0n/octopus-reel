/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef KMEANS_TB_H
#define KMEANS_TB_H

#include <QtGui>
#include "octopus_seg_master.h"

class KMeansTB : public QWidget {
 Q_OBJECT
 public:
  KMeansTB(SegMaster *mVol) : QWidget(0,Qt::SubWindow) {
   mv=mVol;
   setGeometry(540,40,500,300); setFixedSize(500,300);

   setWindowTitle("K-Means Clustering");
  }

 private:
  SegMaster *mv;
};

#endif
