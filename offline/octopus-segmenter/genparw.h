/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef GENPAR_W_H
#define GENPAR_W_H

#include <QtGui>
#include <QString>
#include "octopus_seg_master.h"

class GenParW : public QWidget {
 Q_OBJECT
 public:
  GenParW(SegMaster *sm) : QWidget(0,Qt::SubWindow) { p=sm;
   setGeometry(540,40,500,300); setFixedSize(500,300);
   setWindowTitle("General Settings");
  }

 private slots:

 private:
  SegMaster *p; QString dummyString;
};

#endif
