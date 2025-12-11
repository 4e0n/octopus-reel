/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

#pragma once
#include <vector>

// Direct Form II Transposed Single-Sample IIR Filter Class
class IIRFilter {
public:
 IIRFilter(const std::vector<double>& bCoeffs,const std::vector<double>& aCoeffs)
           : b(bCoeffs),a(aCoeffs),order(aCoeffs.size()-1),w(order,0.0f) {}
 double filterSample(double x) {
  double wn=x;
  for (size_t i=0;i<order;++i) wn-=a[i+1]*w[i];
  wn/=a[0];

  double yn=b[0]*wn;
  for (size_t i=0;i<order;++i) yn+=b[i+1]*w[i];

  for (size_t i=order-1;i>0;--i) w[i]=w[i-1];
  w[0]=wn;

  return yn;
 }

private:
 std::vector<double> b,a;
 size_t order;
 std::vector<double> w;
};
