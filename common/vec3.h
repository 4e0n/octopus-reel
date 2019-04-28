/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

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

/* Octopus Project Vec3 3D Vector Class
    Implements regular 3D (space) vectors including the common inner scalar
    product (2 norm) and the cross product. */

#ifndef OCTOPUS_VEC3_H
#define OCTOPUS_VEC3_H

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
//  friend inline Vec3 operator/(Vec3 a,const float& b) {
//   a[0]/=b; a[1]/=b; a[2]/=b; return a;
//  }
 public:

  inline Vec3() : std::valarray<float>(3) {} // Empty Constructor

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

  // Rotate the vector (*this) in positive mathematical direction around the
  // direction given by vector r.
  // The norm of vector r specifies the rotation angle in radians.
  Vec3 rotate(const Vec3& r) {
   float phi=norm(r);
   if (phi!=0) {
    Vec3 par(r*(*this)/(r*r) * r); // Part parallel to r
    Vec3 perp(*this - par);        // Part perpendicular to r
    Vec3 rotdir(norm(perp) * normalize(cross(r,perp))); // Rotation direction
    return Vec3(par+cos(phi)*perp+sin(phi)*rotdir);
   } else {
    std::cout << "Vec3: rotation angle is zero!" << std:: endl; return *this;
   }
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
   // If |a|=0 or |b|=0, then angle is not defined.
   // We return 1.0 in this case and print error..
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

  // Returns the angle between vectors a and b, but with respect to a
  // preferred rotation direction c.
  // * Params vector a and be must be |a|>0 & |b|>0 or NaN is returned.
  // * Param vector c indicates the rotation direction. It can be any vector
  //   which is not part of the plane spanned by a and b. If |c|=0, the
  //   smallest possible angle is returned.
  //   The angle is returned in radians between 0 and 2*Pi,
  //   or NaN if |a|=0 or |b|=0.
  //
  // * For vector a being not parallel to vector b, and vector a not
  //   antiparallel to vector b, the two vectors a and b span a unique plane
  //   in 3D space.
  //   Let vectors n1 and n2 be the two possible normal vectors for this plane
  //   with |ni|=1, i={1,2} and n1=-n2.
  //
  // * Let further vectors a and b enclose an angle alpha in [0,Pi],
  //   then there is one i in {1,2} such that (alpha*ni x a ) * b=0.
  //   This means vector a rotated by the rotation vector alpha*ni is parallel
  //   to vector b. One could also rotate vector a vy (2*Pi-alpha)*(-ni) to
  //   accomplish the same transformation with ((2*Pi-alpha)*(-ni) x a)*b = 0.
  //
  // * The vector c defines the direction of the normal vector to take as
  //   reference. If c*ni>0, alpha is returned and otherwise 2*Pi-alpha.
  //   If vector a is parallel to vector b, the choice of vector c does not
  //   matter.
  static float angle2(const Vec3& a,const Vec3& b,const Vec3& c) {
   // If |a|=0 or |b|=0, then angle is not defined. We return NaN in this case.
   float ang=angle(a,b); return (cross(a,b)*c < 0) ? float(2.*M_PI)-ang : ang;
  }

  float angle2(const Vec3& b,const Vec3& c) {
   // If |a|=0 or |b|=0, then angle is not defined. We return NaN in this case.
   float ang=angle((*this),b);
   return (cross((*this),b)*c < 0) ? float(2.*M_PI)-ang : ang;
  }

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
