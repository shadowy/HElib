/* Copyright (C) 2012-2017 IBM Corp.
 * This program is Licensed under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. See accompanying LICENSE file.
 */
/* sample.cpp - implementing various sampling routines */
#include <vector>
#include <cassert>
#include <NTL/ZZX.h>
#include <NTL/ZZ_pX.h>
#include <NTL/BasicThreadPool.h>
#include "sample.h"
#include "NumbTh.h"
NTL_CLIENT

// Sample a degree-(n-1) poly, with only Hwt nonzero coefficients
void sampleHWt(zzX &poly, long n, long Hwt)
{
  if (n<=0) n=lsize(poly); if (n<=0) return;
  if (Hwt>=n) {
#ifdef DEBUG_PRINTOUT
    std::cerr << "Hwt="<<Hwt<<">=n="<<n<<", is this ok?\n";
#endif
    Hwt = n-1;
  }
  poly.SetLength(n); // allocate space
  for (long i=0; i<n; i++) poly[i] = 0;

  long i=0;
  while (i<Hwt) {  // continue until exactly Hwt nonzero coefficients
    long u = NTL::RandomBnd(n);  // The next coefficient to choose
    if (poly[u]==0) { // if we didn't choose it already
      long b = NTL::RandomBits_long(2)&2; // b random in {0,2}
      poly[u] = b-1;                      //   random in {-1,1}

      i++; // count another nonzero coefficient
    }
  }
}
// Sample a degree-(n-1) ZZX, with only Hwt nonzero coefficients
void sampleHWt(ZZX &poly, long n, long Hwt)
{
  zzX pp;
  sampleHWt(pp, n, Hwt);
  convert(poly, pp);
}

// Sample a degree-(n-1) ZZX, with -1/0/+1 coefficients
void sampleSmall(zzX &poly, long n)
{
  if (n<=0) n=lsize(poly); if (n<=0) return;
  poly.SetLength(n);

  NTL_EXEC_RANGE(n, first, last)
  for (long i=first; i<last; i++) {
    long u = NTL::RandomBits_long(2);
    if (u&1) poly[i] = (u & 2) -1; // with prob. 1/2 choose between +-1
    else poly[i] = 0;
  }
  NTL_EXEC_RANGE_END
}
// Sample a degree-(n-1) ZZX, with -1/0/+1 coefficients
void sampleSmall(ZZX &poly, long n)
{  
  zzX pp;
  sampleSmall(pp, n);
  convert(poly.rep, pp);
  poly.normalize();
}

// Choose a vector of continuous Gaussians
void sampleGaussian(std::vector<double> &dvec, long n, double stdev)
{
  static double const Pi=4.0*atan(1.0);  // Pi=3.1415..
  static double const bignum = LONG_MAX; // convert to double
  // THREADS: C++11 guarantees these are initialized only once

  if (n<=0) n=lsize(dvec); if (n<=0) return;
  dvec.resize(n, 0.0);        // allocate space for n variables

  // Uses the Box-Muller method to get two Normal(0,stdev^2) variables
  for (long i=0; i<n; i+=2) {
    // r1, r2 are "uniform in (0,1)"
    double r1 = (1+NTL::RandomBnd(LONG_MAX))/(bignum+1);
    double r2 = (1+NTL::RandomBnd(LONG_MAX))/(bignum+1);
    double theta=2*Pi*r1;
    double rr= sqrt(-2.0*log(r2))*stdev;
    if (rr > 8*stdev) // sanity-check, trancate at 8 standard deviations
      rr = 8*stdev;

    // Generate two Gaussians RV's
    dvec[i] = rr*cos(theta);
    if (i+1 < n)
      dvec[i+1] = rr*sin(theta);
  }
}

// Sample a degree-(n-1) ZZX, with rounded Gaussian coefficients
void sampleGaussian(zzX &poly, long n, double stdev)
{
  if (n<=0) n=lsize(poly); if (n<=0) return;
  std::vector<double> dvec;
  sampleGaussian(dvec, n, stdev); // sample continuous Gaussians

  // round and copy to coefficients of poly
  clear(poly);
  poly.SetLength(n); // allocate space for degree-(n-1) polynomial
  for (long i=0; i<n; i++)
    poly[i] = long(round(dvec[i])); // round to nearest integer
}
// Sample a degree-(n-1) ZZX, with rounded Gaussian coefficients
void sampleGaussian(ZZX &poly, long n, double stdev)
{
  zzX pp;
  sampleGaussian(pp, n, stdev);
  convert(poly.rep, pp);
  poly.normalize();
}
#if 0
void sampleGaussian(ZZX &poly, long n, double stdev)
{
  static double const Pi=4.0*atan(1.0); // Pi=3.1415..
  static long const bignum = 0xfffffff;
  // THREADS: C++11 guarantees these are initialized only once

  if (n<=0) n=deg(poly)+1; if (n<=0) return;
  poly.SetMaxLength(n); // allocate space for degree-(n-1) polynomial
  for (long i=0; i<n; i++) SetCoeff(poly, i, ZZ::zero());

  // Uses the Box-Muller method to get two Normal(0,stdev^2) variables
  for (long i=0; i<n; i+=2) {
    double r1 = (1+RandomBnd(bignum))/((double)bignum+1);
    double r2 = (1+RandomBnd(bignum))/((double)bignum+1);
    double theta=2*Pi*r1;
    double rr= sqrt(-2.0*log(r2))*stdev;

    assert(rr < 8*stdev); // sanity-check, no more than 8 standard deviations

    // Generate two Gaussians RV's, rounded to integers
    long x = (long) floor(rr*cos(theta) +0.5);
    SetCoeff(poly, i, x);
    if (i+1 < n) {
      x = (long) floor(rr*sin(theta) +0.5);
      SetCoeff(poly, i+1, x);
    }
  }
  poly.normalize(); // need to call this after we work on the coeffs
}
#endif

// Sample a degree-(n-1) zzX, with coefficients uniform in [-B,B]
void sampleUniform(zzX& poly, long n, long B)
{
  assert (B>0);
  if (n<=0) n=lsize(poly); if (n<=0) return;
  poly.SetLength(n); // allocate space for degree-(n-1) polynomial

  for (long i = 0; i < n; i++)
    poly[i] = NTL::RandomBnd(2*B +1) - B;
}

// Sample a degree-(n-1) ZZX, with coefficients uniform in [-B,B]
void sampleUniform(ZZX& poly, long n, const ZZ& B)
{
  assert (B>0);
  if (n<=0) n=deg(poly)+1; if (n<=0) return;
  clear(poly);
  poly.SetMaxLength(n); // allocate space for degree-(n-1) polynomial

  ZZ UB = 2*B +1;
  for (long i = n-1; i >= 0; i--) {
    ZZ tmp = RandomBnd(UB) - B;
    SetCoeff(poly, i, tmp);
  }
}


#include <mutex>
#include <map>
#include "PAlgebra.h"
// Helper functions, returns a zz_pXModulus object, modulo Phi_m(X)
// and a single 60-bit prime. Can be used to get fatser operation
// modulo Phi_m(X), where we know apriori that the numbers do not wrap.
// This function changes the NTL current zz_p modulus.
const zz_pXModulus& getPhimXMod(const PAlgebra& palg)
{
  static std::map<long,zz_pXModulus*> moduli; // pointer per value of m
  static std::mutex pt_mtx;      // control access to modifying the map

  zz_p::FFTInit(0); // set "the best FFT prime" as NTL's current modulus

  long m = palg.getM();
  auto it = moduli.find(m); // check if we already have zz_pXModulus for m

  if (it==moduli.end()) {   // init a new zz_pXModulus for this value of m
    std::unique_lock<std::mutex> lck(pt_mtx); // try to get a lock

    // Got the lock, insert a new entry for this malue of m into the map
    zz_pX phimX = conv<zz_pX>(palg.getPhimX());
    zz_pXModulus* ptr = new zz_pXModulus(phimX); // will "never" be deleted

    // insert returns a pair (iterator, bool)
    auto ret = moduli.insert(std::pair<long,zz_pXModulus*>(m,ptr));
    if (ret.second==false) // Another thread interted it, delete your copy
      delete ptr;
    // FIXME: Could leak memory if insert throws an exception
    //        without inserting the element (but who cares)

    it = ret.first; // point to the entry in the map
  }
  return *(it->second);
}

// DIRT: We use modular arithmetic mod p \approx 2^{60} as a
//       substitute for computing on rational numbers
static void reduceModPhimX(zzX& poly, const PAlgebra& palg)
{
  zz_pPush push; // backup the NTL current modulus
  const zz_pXModulus& phimX = getPhimXMod(palg);

  zz_pX pp;
  convert(pp, poly);   // convert to zz_pX
  rem(pp, pp, phimX);
  convert(poly, pp);
}

/********************************************************************
 * Below are versions of the sampling routines that sample modulo
 * X^m-1 and then reduce mod Phi_m(X). The exception is when m is
 * a power of two, where we still sample directly mod Phi_m(X).
 ********************************************************************/
void sampleHWt(zzX &poly, const PAlgebra& palg, long Hwt)
{
  if (palg.getPow2() > 0) { // not power of two
    sampleHWt(poly, palg.getM(), Hwt);
    reduceModPhimX(poly, palg);
  }
  else // power of two
    sampleHWt(poly, palg.getPhiM(), Hwt);
}
void sampleSmall(zzX &poly, const PAlgebra& palg)
{
  if (palg.getPow2() > 0) { // not power of two
    sampleSmall(poly, palg.getM());
    reduceModPhimX(poly, palg);
  }
  else // power of two
    sampleSmall(poly, palg.getPhiM());
}
void sampleGaussian(zzX &poly, const PAlgebra& palg, double stdev)
{
  if (palg.getPow2() > 0) { // not power of two
    sampleGaussian(poly, palg.getM(), stdev);
    reduceModPhimX(poly, palg);
  }
  else // power of two
    sampleGaussian(poly, palg.getPhiM(), stdev);
}
void sampleUniform(zzX &poly, const PAlgebra& palg, long B)
{
  if (palg.getPow2() > 0) { // not power of two
    sampleUniform(poly, palg.getM(), B);
    reduceModPhimX(poly, palg);
  }
  else // power of two
    sampleUniform(poly, palg.getPhiM(), B);
}


// Implementing the Ducas-Durmus error procedure
void sampleErrorDD(zzX& err, const PAlgebra& palg, double stdev)
{
  static long const factor = 1L<<32;

  long m = palg.getM();

  // Choose a continuous Gaussiam mod X^m-1, with param srqt(m)*stdev
  std::vector<double> dvec;
  sampleGaussian(dvec, m, stdev * sqrt(double(m)));

  // Now we need to reduce it modulo Phi_m(X), then round to integers

  // Since NTL doesn't support polynomial arithmetic with floating point
  // polynomials, we scale dvec up to by 32 bits and use zz_pX arithmetic,
  // then scale back down and round after the modular reduction.

  err.SetLength(m, 0); // allocate space
  for (long i=0; i<m; i++)
    err[i] = round(dvec[i]*factor);

  reduceModPhimX(err, palg);

  for (long i=0; i<lsize(err); i++) { // scale down and round
    err[i] += factor/2;
    err[i] /= factor;
  }
}
void sampleErrorDD(ZZX& err, const PAlgebra& palg, double stdev)
{
  zzX pp;
  sampleErrorDD(pp, palg, stdev);
  convert(err.rep, pp);
  err.normalize();
}