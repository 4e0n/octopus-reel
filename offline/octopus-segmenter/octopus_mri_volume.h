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
