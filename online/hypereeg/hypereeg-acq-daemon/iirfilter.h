#pragma once
#include <vector>

// Direct Form II Transposed Single-Sample IIR Filter Class
class IIRFilter {
public:
 IIRFilter(const std::vector<float>& bCoeffs,const std::vector<float>& aCoeffs)
           : b(bCoeffs),a(aCoeffs),order(aCoeffs.size()-1),w(order,0.0f) {}
 float filterSample(float x) {
  float wn=x;
  for (size_t i=0;i<order;++i) wn-=a[i+1]*w[i];
  wn/=a[0];

  float yn=b[0]*wn;
  for (size_t i=0;i<order;++i) yn+=b[i+1]*w[i];

  for (size_t i=order-1;i>0;--i) w[i]=w[i-1];
  w[0]=wn;

  return yn;
 }

private:
 std::vector<float> b,a;
 size_t order;
 std::vector<float> w;
};

// Usage Example inside your AcqThread
// Create per-channel filter objects at initialization:
// std::vector<IIRFilter> channelFilters;
// for (size_t ch=0;ch<totalChannels;++ch) channelFilters.emplace_back(b,a);

// Then for each sample:
// float rawValue=tcpS.amp[ampIdx].data[chIdx];
// float filteredValue=channelFilters[chGlobalIdx].filterSample(rawValue);
// tcpS.amp[ampIdx].data[chIdx] = filteredValue;

// Proceed to push tcpS into your circular buffer as before.
