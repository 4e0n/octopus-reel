/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_MRI_VOLUME_H
#define OCTOPUS_MRI_VOLUME_H

#include <QVector>
#include "octopus_mri_slice.h"

class MRIVolume {
 public:
  MRIVolume() {}
  ~MRIVolume() { clear(); }

  void clear() {
   for (int i=0;i<slice.size();i++) delete slice[i]; slice.resize(0);
  }

  void append(MRISlice *s) { slice.append(s); }

  void updateHistogram() { float t;
   histogram.resize(slice.size());
   for (int i=0;i<256;i++) { t=0.;
    for (int j=0;j<slice.size();j++) t+=slice[j]->histogram[i];
    histogram[i]=t/(float)(slice.size());
   }
  }

  QVector<MRISlice*> slice; QVector<float> histogram;
};

#endif
