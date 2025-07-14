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

// Usage Example inside your AcqThread
// Create per-channel filter objects at initialization:
// std::vector<IIRFilter> channelFilters;
// for (size_t ch=0;ch<totalChannels;++ch) channelFilters.emplace_back(b,a);

// Then for each sample:
// double rawValue=tcpS.amp[ampIdx].data[chIdx];
// double filteredValue=channelFilters[chGlobalIdx].filterSample(rawValue);
// tcpS.amp[ampIdx].data[chIdx] = filteredValue;

// Proceed to push tcpS into your circular buffer as before.
