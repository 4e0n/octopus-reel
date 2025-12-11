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

#include <cstdio>

// If OCTO_OMP is defined before this header, OMP will be tried to be enabled
// Otherwise falling back to serial loops

#if defined(OCTO_OMP) && defined(_OPENMP)
 #include <omp.h>
 #define OCTO_OMP_ENABLED 1
 // 1D parallel for
 #define PARFOR(i,begin,end) _Pragma("omp parallel for schedule(static)") for (int i=int(begin);i<int(end);++i)

 // 2D collapsed parallel for (e.g., amp × chn)
 #define PARFOR2(i,j,ib,ie,jb,je) _Pragma("omp parallel for collapse(2) schedule(static)") for (int i=int(ib);i<int(ie);++i) for (int j=int(jb);j<int(je);++j)

 // SIMD hint with + reduction
 #define SIMD_REDUCE_PLUS(var) _Pragma("omp simd reduction(+:var)")
 #define OMP_PAR_BEGIN         _Pragma("omp parallel")
 #define OMP_FOR               _Pragma("omp for schedule(static)")
 #define OMP_CRITICAL          _Pragma("omp critical")
 #define OMP_BARRIER           _Pragma("omp barrier")
#else
 #define OCTO_OMP_ENABLED 0
 #define PARFOR(i,begin,end) for (int i=int(begin);i<int(end);++i)
 #define PARFOR2(i,j,ib,ie,jb,je) for (int i=int(ib);i<int(ie);++i) for (int j=int(jb);j<int(je);++j)
 #define SIMD_REDUCE_PLUS(var)
 #define OMP_PAR_BEGIN
 #define OMP_FOR
 #define OMP_CRITICAL
 #define OMP_BARRIER
#endif

// Runtime diagnostics
inline void omp_diag() {
#if defined(OCTO_OMP) && defined(_OPENMP)
 // _OPENMP is a yyyymm integer (e.g., 201811 for OpenMP 5.0)
 std::printf("[OMP] enabled, _OPENMP=%d\n",_OPENMP);
 std::printf("[OMP] omp_get_max_threads() = %d\n",omp_get_max_threads());
 std::printf("[OMP] omp_get_dynamic()     = %d\n",omp_get_dynamic());
 std::printf("[OMP] omp_get_nested()      = %d\n",omp_get_nested());

 int spawned=1,inpar=0;
 #pragma omp parallel
 {
  #pragma omp single
  {
   spawned=omp_get_num_threads();
   inpar=omp_in_parallel();
  }
 }
 std::printf("[OMP] spawned threads       = %d (in_parallel=%d)\n",spawned,inpar);
#elif defined(OCTO_OMP) && !defined(_OPENMP)
 std::printf("[OMP] OCTO_OMP defined, but compiler not built with OpenMP. Falling back to serial.\n");
#else
 std::printf("[OMP] OCTO_OMP not defined. Serial build.\n");
#endif
}

// Tiny helper to set threads at runtime (no-op without OpenMP)
inline void omp_set_threads_if_available(int n) {
#if OCTO_OMP_ENABLED
 if (n>0) omp_set_num_threads(n);
#else
 (void)n;
#endif
}

// helper to use _Pragma with macro args
#define OMP_PRAGMA(x) _Pragma(#x)

#if defined(OCTO_OMP) && defined(_OPENMP)
 // single / dual variable simd reduction
 #undef  SIMD_REDUCE_PLUS
 #define SIMD_REDUCE1(a)   OMP_PRAGMA(omp simd reduction(+:a))
 #define SIMD_REDUCE2(a,b) OMP_PRAGMA(omp simd reduction(+:a,b))
#else
 #undef  SIMD_REDUCE_PLUS
 #define SIMD_REDUCE1(a)
 #define SIMD_REDUCE2(a,b)
#endif
