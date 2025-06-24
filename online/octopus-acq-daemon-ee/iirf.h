#ifndef IIRF_H
#define IIRF_H

class IIRF {
 public:
  //IIRF_HPF(const std::array<double,3>& b_coeffs,const std::array<double,3>& a_coeffs):b(b_coeffs),a(a_coeffs) {}
  //IIRF_BPF(const std::array<double,5>& b_coeffs,const std::array<double,5>& a_coeffs):b(b_coeffs),a(a_coeffs) {}
  IIRF() {}
  double hpf_process(double x) { // Process single sample
   double y=(hp_b[0]*x+hp_b[1]*xh[0]+hp_b[2]*xh[1])-(hp_a[1]*yh[0]+hp_a[2]*yh[1]);
   // Update history
   xh[1]=xh[0]; xh[0]=x;
   yh[1]=yh[0]; yh[0]=y;
   return y;
  }
  double bpf_process(double x) { // Process single sample
   double y=(bp_b[0]*x+bp_b[1]*xh[0]+bp_b[2]*xh[1]+bp_b[3]*xh[2]+bp_b[4]*xh[3])-(bp_a[1]*yh[0]+bp_a[2]*yh[1]+bp_a[3]*yh[2]+bp_a[4]*yh[3]);
   // Update history
   xh[3]=xh[2]; xh[2]=xh[1]; xh[1]=xh[0]; xh[0]=x;
   yh[3]=yh[2]; yh[2]=yh[1]; yh[1]=yh[0]; yh[0]=y;
   return y;
  }
  void hpf_processBuffer(std::vector<double>& signal) { // Process chunk - forward filtering
   for (size_t i=0;i<signal.size();i++) { signal[i]=hpf_process(signal[i]); }
  }
  void bpf_processBuffer(std::vector<double>& signal) { // Process chunk - forward filtering
   for (size_t i=0;i<signal.size();i++) { signal[i]=bpf_process(signal[i]); }
  }
  void hpf_processFiltFilt(std::vector<double>& signal,eex& e,int chn) {
   for (unsigned int i=0;i<2;i++) { xh[i]=e.fX[chn][i]; yh[i]=e.fX[chn][i]; }
   hpf_processBuffer(signal); // Forward
   std::reverse(signal.begin(),signal.end()); hpf_processBuffer(signal); // Reverse
   std::reverse(signal.begin(),signal.end());
   for (unsigned int i=0;i<2;i++) { e.fX[chn][i]=xh[i]; e.fX[chn][i]=yh[i]; }
  }
  void bpf_processFiltFilt(std::vector<double>& signal,eex& e,int chn) {
   //if (e.idx==0 && chn==0) { for (unsigned int i=0;i<signal.size();i++) printf("%f ",signal[i]); printf("\n"); }
   if (e.idx==0 && chn==0) {
    for (unsigned int i=0;i<e.fX[0].size();i++) printf("%f ",e.fX[chn][i]);
    for (unsigned int i=0;i<e.fY[0].size();i++) printf("%f ",e.fY[chn][i]);
    printf("\n");
   }
   for (unsigned int i=0;i<4;i++) { xh[i]=e.fX[chn][i]; yh[i]=e.fY[chn][i]; }
   hpf_processBuffer(signal); // Forward
   //std::reverse(signal.begin(),signal.end()); processBuffer(signal); // Reverse
   //std::reverse(signal.begin(),signal.end());
   for (unsigned int i=0;i<4;i++) { e.fX[chn][i]=xh[i]; e.fY[chn][i]=yh[i]; }
  }

 private:
  std::array<double,3> hp_b={ 0.9956,-1.9911, 0.9956};
  std::array<double,3> hp_a={ 1.0   ,-1.9911, 0.9912};
  //std::array<double,3> b; // Numerator coefficients
  //std::array<double,3> a; // Denominator coefficients
  std::array<double,5> bp_b={ 0.0127, 0.0   ,-0.0255, 0.0   , 0.0127};
  std::array<double,5> bp_a={ 1.0000,-3.6533, 5.0141,-3.0680, 0.7071};
  //std::array<double,5> b; // Numerator coefficients
  //std::array<double,5> a; // Denominator coefficients
  std::array<double,4> xh={0,0,0,0}; // Input history
  std::array<double,4> yh={0,0,0,0}; // Output history
};

#endif
