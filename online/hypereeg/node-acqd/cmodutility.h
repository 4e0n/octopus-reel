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

const unsigned int CBUF_SIZE_IN_SECS=10;

static inline float median_inplace(std::vector<float>& v) {
 if (v.empty()) return 0.f;
 const size_t n=v.size()/2;
 std::nth_element(v.begin(),v.begin()+n,v.end());
 float med=v[n];
 if ((v.size()&1)==0) {
  std::nth_element(v.begin(),v.begin()+n-1,v.end());
  med=0.5f*(med+v[n-1]);
 }
 return med;
}

static inline uint8_t map01_to_u8(float x01) {
 if (x01<=0.f) return 0;
 if (x01>=1.f) return 255;
 return static_cast<uint8_t>(x01*255.f+0.5f);
}

static constexpr float CM_ALPHA =0.5f;  // EMA smoothing
static constexpr float S_FLOOR  =0.5f;  // normalize start
static constexpr float S_HIGH   =3.0f;  // normalize end
static constexpr float EPS      =1e-12f;

static constexpr float BAD_ON   =1.0f;  // become BAD when Ssmooth crosses this
static constexpr float BAD_OFF  =0.6f;  // return to GOOD when Ssmooth falls below this

// No flicker around the threshold: channels must clearly cross BAD_ON to become bad, and fall below BAD_OFF to recover.
// The “all-good” cap won’t mute a channel that’s latched BAD (so you still see it).
// Visuals stay continuous; the small “min 0.35 when BAD” line is optional if you want a guaranteed yellow-ish tint while latched.

// Tuning tips
// - Increase BAD_ON or reduce BAD_OFF to make latching stricter (less toggling).
// - If the map still feels chatty when good, raise S_FLOOR to ~0.8 or lower S_HIGH to ~2.5.
// - If recovery feels slow, raise BAD_OFF a bit (e.g., 0.7).
