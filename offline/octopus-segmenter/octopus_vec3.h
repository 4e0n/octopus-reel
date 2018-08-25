/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef VEC3_H
#define VEC3_H

#include <iostream>
#include <valarray>
#include <cmath>

class Vec3 : public std::valarray<float> {

  /* Inline non-member operators */

  friend inline Vec3 operator+(Vec3 a,const Vec3& b) {
   a[0]+=b[0]; a[1]+=b[1]; a[2]+=b[2]; return a;
  }
  friend inline Vec3 operator-(Vec3 a,const Vec3& b) {
   a[0]-=b[0]; a[1]-=b[1]; a[2]-=b[2]; return a;
  }
  friend inline Vec3 operator-(Vec3 a) {
   a[0]=-a[0]; a[1]=-a[1]; a[2]=-a[2]; return a;
  }
  friend inline float operator*(Vec3 a,const Vec3& b) {
   a[0]*=b[0]; a[1]*=b[1]; a[2]*=b[2]; return a[0]+a[1]+a[2];
  }
  friend inline Vec3 operator*(const float& a,Vec3 b) {
   b[0]*=a; b[1]*=a; b[2]*=a; return b;
  }
  friend inline Vec3 operator*(Vec3 a,const float& b) {
   a[0]*=b; a[1]*=b; a[2]*=b; return a;
  }

 public:

  inline Vec3() : std::valarray<float>(3) { // Empty Constructor
   (*this)[0]=(*this)[1]=(*this)[2]=0.;
  }

  inline Vec3(const float& a, // Constructor with initial elements
              const float& b,
              const float& c) : std::valarray<float>(3) {
   (*this)[0]=a; (*this)[1]=b; (*this)[2]=c;
  }

  // Copy constructor
  inline Vec3(const std::valarray<float>& a) : std::valarray<float>(a) {}
  inline Vec3(const std::slice_array<float>& a) : std::valarray<float>(a) {}

  // Operators and other routines

  void zero() { (*this)[0]=(*this)[1]=(*this)[2]=0.; }

  // Norm of argument vector.
  static float norm(const Vec3& a) { return sqrt(a*a); }
  float norm() { return sqrt((*this)*(*this)); }

  // Cross product of vectors a and b.
  static Vec3 cross(const Vec3& a,const Vec3& b) {
   return Vec3(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);
  }

  void cross(const Vec3& b) {
   Vec3 a=Vec3((*this)[1]*b[2]-(*this)[2]*b[1],
               (*this)[2]*b[0]-(*this)[0]*b[2],
               (*this)[0]*b[1]-(*this)[1]*b[0]);
   (*this)[0]=a[0]; (*this)[1]=a[1]; (*this)[2]=a[2];
  }

//  Vec3 cross(const Vec3& b) {
//   return Vec3((*this)[1]*b[2]-(*this)[2]*b[1],
//               (*this)[2]*b[0]-(*this)[0]*b[2],
//               (*this)[0]*b[1]-(*this)[1]*b[0]);
//  }

  // Normalize the vector to have a norm of 1.
  // Returns the normalized vector if the length is non-zero,
  //  otherwise the zero-vector.
  static Vec3 normalize(const Vec3& a) { float n=norm(a);
   if (n!=0.) return Vec3(a[0]/n,a[1]/n,a[2]/n);
   else { std::cout << "Vec3: Unable to Normalize. 0-size vector!" <<
          std:: endl; return a; }
  }

  void normalize() { float n;
   if ((n=norm(*this)) != 0.) { (*this)[0]/=n; (*this)[1]/=n; (*this)[2]/=n; }
   else std::cout << "Vec3: Unable to Normalize. 0-size vector!" << std::endl;
  }

  void rotX(const float angle) {
   float c=cos(angle),s=sin(angle),y=(*this)[1],z=(*this)[2];
   (*this)[1]=y*c-z*s; (*this)[2]=y*s+z*c;
  }
  void rotY(const float angle) {
   float c=cos(angle),s=sin(angle),x=(*this)[0],z=(*this)[2];
   (*this)[0]=x*c+z*s; (*this)[2]=-x*s+z*c;
  }
  void rotZ(const float angle) {
   float c=cos(angle),s=sin(angle),x=(*this)[0],y=(*this)[1];
   (*this)[0]=x*c-y*s; (*this)[1]=x*s+y*c;
  }

  // STATIC FUNCTIONS

  // Cosine of the angle between vectors a and b.
  static float cosine(const Vec3& a,const Vec3& b) {
   float den=norm(a)*norm(b); float ret=0.;
   // If |a|=0 or |b|=0, then angle is not defined.
   // We return 1.0 in this case and print error..
   if (den!=0.) ret=a*b/den; else { ret=1.;
    std::cout << "Vec3: One of the cosine norms are zero!" << std:: endl;
   } return ret;
  }

  float cosine(const Vec3& b) {
   float den=norm((*this))*norm(b); float ret=0.;
   if (den!=0.) ret=(*this)*b/den; else { ret=1.;
    std::cout << "Vec3: One of the cosine norms are zero!" << std:: endl;
   } return ret;
  }

  // Angle between vectors a and b.
  // Norms of vectors must be > 0 otherwise NaN is returned.
  // Returns the angle in radians between 0 and Pi,
  //  or returns NaN if |a|=0 or |b|=0.
  static float angle(const Vec3& a,const Vec3& b) {
   if (!(a[0]==b[0] && a[1]==b[1] && a[2]==b[2])) return acos(cosine(a,b));
   else return 0.0;
  }

  float angle(const Vec3& b) { return acos(cosine((*this),b)); }

  // Spherical coords..
  float sphR() { return sqrt((*this)[0]*(*this)[0]+
                             (*this)[1]*(*this)[1]+
                             (*this)[2]*(*this)[2]); }
  float sphTheta() { return acos((*this)[2]/sphR()); }
  float sphPhi() { return atan2((*this)[1],(*this)[0]); }

  Vec3 del(Vec3 x1,Vec3 x2,Vec3 x3) { Vec3 r;
   r=(x1+x2+x3)*(1./3.)-(*this); return r;
  }

  Vec3 del2(Vec3 x1,Vec3 x2,Vec3 x3,
            Vec3 x4,Vec3 x5,Vec3 x6,
            Vec3 x7,Vec3 x8,Vec3 x9) { Vec3 r,s1,s2,s3;
   s1=x4+x5+x6+x7+x8+x9; s2=6.*(x1+x2+x3); s3=6.*(*this);
   r=(s1+s2+s3)*(1./9.); return r;
  }

  void print() {
   printf("%2.2f %2.2f %2.2f\n",(*this)[0],(*this)[1],(*this)[2]);
  }
};

#endif
