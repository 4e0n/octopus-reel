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

#pragma once
#include <algorithm>
#include <cmath>

struct AudRelState {
  float emaEnv = 1e-6f;   // EMA of linear envelope (0..1)
};

inline float safe_env(float e) { return (e > 1e-7f) ? e : 1e-7f; }

// Update EMA of envelope (linear). alphaMean: 0.12..0.30 (higher = snappier)
inline void audrel_update_mean(AudRelState& st, float envLinear, float alphaMean=0.20f) {
  float e = safe_env(envLinear);
  st.emaEnv = (1.f - alphaMean)*st.emaEnv + alphaMean*e;
}

// Relative dB around current mean: +dB if louder than mean, -dB if softer
inline float audrel_db(float envLinear, const AudRelState& st) {
  return 20.f * std::log10( safe_env(envLinear) / safe_env(st.emaEnv) );
}

// Map ±span_dB to ±H/2 pixels (zero at mid)
inline int audrel_db_to_px(float rel_dB, float span_dB, int H) {
  const float half = 0.5f * std::max(6.f, span_dB);      // min span 6 dB
  float d = std::max(-half, std::min(half, rel_dB));     // clamp
  const float gain = (H * 0.5f) / half;                  // ±half → ±H/2
  return int(d * gain);
}


// --- Symmetric linear envelope (zero-centered) -----------------------------
struct AudSymLin {
  float emaEnv = 1e-6f;   // mean of envelope (linear 0..1)
  float emaAbs = 1e-6f;   // mean absolute deviation of envelope
};

// Update EMAs and return the signed deviation (env - mean)
inline float audsym_update(AudSymLin& st, float envLinear,
                           float alphaMean = 0.20f,  // 0.15..0.30
                           float alphaAbs  = 0.20f)  // match alphaMean
{
  const float e = (envLinear > 1e-7f) ? envLinear : 1e-7f;
  st.emaEnv = (1.f - alphaMean)*st.emaEnv + alphaMean*e;
  const float dev = e - st.emaEnv;
  st.emaAbs = (1.f - alphaAbs )*st.emaAbs  + alphaAbs *std::fabs(dev);
  return dev; // signed
}

// Map a signed deviation to pixels with symmetric scaling.
// 'k' controls sensitivity: larger k => bigger swings (try 3..6).
inline int audsym_dev_to_px(float dev, const AudSymLin& st, int H, float k = 4.0f)
{
  const float span = std::max(1e-7f, k * st.emaAbs); // symmetric ±span
  float d = std::max(-span, std::min(span, dev));
  const float gain = (H * 0.5f) / span;              // ±span -> ±H/2
  return int(d * gain);
}
