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

struct AcqChnInfo {
 unsigned int physChn; QString chnName;
 unsigned int type; // 0:ref,1:bipolar,2:meta;
 float topoTheta,topoPhi; unsigned int topoX,topoY;
 QVector<unsigned int> interMode; // for each amp
 QVector<unsigned int> interElec; // Neighboring electrodes

 // Involved channels
 // Operation (avg,gfp,difference, etc.)
};
