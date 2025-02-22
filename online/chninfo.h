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

#ifndef _CHNINFO_H
#define _CHNINFO_H

typedef struct _chninfo {
 unsigned int sampleRate;
 unsigned int refChnCount;
 unsigned int bipChnCount;
 unsigned int physChnCount;
 unsigned int refChnMaxCount;
 unsigned int bipChnMaxCount;
 unsigned int physChnMaxCount;
 unsigned int totalChnCount;
 unsigned int totalCount; // Chncount among all connected amplifiers
 unsigned int probe_eeg_msecs;
 unsigned int probe_impedance_msecs;
} chninfo;

#endif
