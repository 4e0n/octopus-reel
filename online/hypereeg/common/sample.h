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
#include <QDataStream>

struct Sample {
 std::vector<float> dataR; // Raw (non-filtered) amplifier data.
 std::vector<float> dataBP;  // [2-40]Hz IIR filtered amplifier data.
 std::vector<float> dataN; // 30Q @ 50Hz IIR notch filtered version of data
 std::vector<float> dataD; // [2-4]Hz IIR filtered Delta data.
 std::vector<float> dataT; // [4-8]Hz IIR filtered Theta data.
 std::vector<float> dataA; // [8-14]Hz IIR filtered Alpha data.
 std::vector<float> dataB; // [14-28]Hz IIR filtered Beta data.
 std::vector<float> dataG; // [28-40]Hz IIR filtered Gamma data.
 std::vector<float> sumBP; // Sum [2-40]Hz IIR filtered amplifier data.
 std::vector<float> sumD; // Sum [2-40]Hz IIR filtered amplifier data.
 std::vector<float> sumT; // Sum [2-40]Hz IIR filtered amplifier data.
 std::vector<float> sumA; // Sum [2-40]Hz IIR filtered amplifier data.
 std::vector<float> sumB; // Sum [2-40]Hz IIR filtered amplifier data.
 std::vector<float> sumG; // Sum [2-40]Hz IIR filtered amplifier data.
 unsigned int trigger=0,offset=0;

 void init(size_t chnCount) {
  trigger=0; offset=0; dataR.assign(chnCount,0.0f); dataBP.assign(chnCount,0.0f); dataN.assign(chnCount,0.0f);
  dataD.assign(chnCount,0.0f); dataT.assign(chnCount,0.0f);
  dataA.assign(chnCount,0.0f); dataB.assign(chnCount,0.0f); dataG.assign(chnCount,0.0f);
  sumBP.assign(chnCount,0.0f);
  sumD.assign(chnCount,0.0f); sumT.assign(chnCount,0.0f);
  sumA.assign(chnCount,0.0f); sumB.assign(chnCount,0.0f); sumG.assign(chnCount,0.0f);
 }

 Sample(size_t chnCount=0) { init(chnCount); }
 
 void serialize(QDataStream &out) const {
  for (float f:dataBP) out<<f;
  for (float f:dataN) out<<f;
  for (float f:dataD) out<<f;
  for (float f:dataT) out<<f;
  for (float f:dataA) out<<f;
  for (float f:dataB) out<<f;
  for (float f:dataG) out<<f;
  for (float f:sumBP) out<<f;
  for (float f:sumD) out<<f;
  for (float f:sumT) out<<f;
  for (float f:sumA) out<<f;
  for (float f:sumB) out<<f;
  for (float f:sumG) out<<f;
 }

 bool deserialize(QDataStream &in,size_t chnCount) {
  dataBP.resize(chnCount); dataN.resize(chnCount);
  dataD.resize(chnCount); dataT.resize(chnCount); dataA.resize(chnCount); dataB.resize(chnCount); dataG.resize(chnCount);
  sumBP.resize(chnCount);
  sumD.resize(chnCount); sumT.resize(chnCount); sumA.resize(chnCount); sumB.resize(chnCount); sumG.resize(chnCount);

  for (float &f:dataBP) in>>f;
  for (float &f:dataN) in>>f;
  for (float &f:dataD) in>>f;
  for (float &f:dataT) in>>f;
  for (float &f:dataA) in>>f;
  for (float &f:dataB) in>>f;
  for (float &f:dataG) in>>f;
  for (float &f:sumBP) in>>f;
  for (float &f:sumD) in>>f;
  for (float &f:sumT) in>>f;
  for (float &f:sumA) in>>f;
  for (float &f:sumB) in>>f;
  for (float &f:sumG) in>>f;
  return true;
 }
};
