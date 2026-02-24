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

#include <QtGlobal> // quint32, quint64, uchar
#include <cstring>  // std::memcpy

// Helpers for little-endian reads (safe'n fast)
static inline quint32 rd_u32_le(const char* p) {
 const uchar* u=reinterpret_cast<const uchar*>(p);
 return (quint32)u[0]
        |((quint32)u[1]<<8)
        |((quint32)u[2]<<16)
        |((quint32)u[3]<<24);
}

static inline quint64 rd_u64_le(const char* p) {
 const uchar* u=reinterpret_cast<const uchar*>(p);
 return (quint64)u[0]
        |((quint64)u[1]<<8)
        |((quint64)u[2]<<16)
        |((quint64)u[3]<<24)
        |((quint64)u[4]<<32)
        |((quint64)u[5]<<40)
        |((quint64)u[6]<<48)
        |((quint64)u[7]<<56);
}

static inline float rd_f32_le(const char* p) {
 quint32 w=rd_u32_le(p);
 float f;
 static_assert(sizeof(float)==4,"float must be 4 bytes");
 std::memcpy(&f,&w,4);
 return f;
}

// ---- Fixed-size wire helpers (local; no le_helper changes needed) ----
static inline void wr_u32_le(char*& p,quint32 v) {
 *p++=char((v    )&0xff);
 *p++=char((v>> 8)&0xff);
 *p++=char((v>>16)&0xff);
 *p++=char((v>>24)&0xff);
}

static inline void wr_u64_le(char*& p,quint64 v) {
 for (int i=0;i<8;++i) { *p++=char((v>>(8*i))&0xff); }
}

static inline void wr_f32_le(char*& p,float f) {
 quint32 w;
 static_assert(sizeof(float)==4,"float must be 32-bit IEEE754");
 std::memcpy(&w,&f,4);
 wr_u32_le(p,w);
}
