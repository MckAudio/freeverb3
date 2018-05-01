/**
 *  Freeverb3 Log Sweep Frequency Response Calculation Example
 *
 *  Copyright (C) 2006-2014 Teru Kamogashira
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "fv3_config.h"

#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdint.h>
#include <freeverb/sweep.hpp>
#include <freeverb/efilter.hpp>
#include <freeverb/biquad.hpp>
#include <freeverb/irmodel3.hpp>
#include <freeverb/utils.hpp>

#ifdef PLUGDOUBLE
#define FFTW_(name) fftw_ ## name
typedef fv3::irmodel3_ IRMODEL3;
typedef fv3::utils_ UTILS;
typedef fv3::sweep_ SWEEP;
typedef fv3::delay_ DELAY;
typedef double pfloat_t;
#else
#define FFTW_(name) fftwf_ ## name
typedef fv3::irmodel3_f IRMODEL3;
typedef fv3::utils_f UTILS;
typedef fv3::sweep_f SWEEP;
typedef fv3::delay_f DELAY;
typedef float pfloat_t;
#endif

int main(int argc, char* argv[])
{
  std::fprintf(stderr, "Freeverb3 Log Sweep Frequency Response Calculation Example\n");
  std::fprintf(stderr, "<" PACKAGE "-" VERSION ">\n");
  std::fprintf(stderr, "Copyright (C) 2006-2014 Teru Kamogashira\n");
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n", (int)sizeof(pfloat_t));

  SWEEP S;
  S.setSampleRate(48000);
  S.setStartFs(0.001);
  S.setEndFs(23000);
  S.setInitialMuteLength(100);
  S.setSweepLength(100);
  S.setLeadInLength(0);
  S.setLeadOutLength(0);
  S.setEndMuteLength(100);
  S.init();
  long totalLength = S.getTotalLength();
  fprintf(stderr, "SweepLength = %ld\n", totalLength);

  std::cerr << "You can use GNUPLOT to plot the Frequency Response and Phase." << std::endl;
  std::cerr << "gnuplot> set logscale" << std::endl;
  std::cerr << "gnuplot> # Plot Frequency Response" << std::endl;
  std::cerr << "gnuplot> plot [:" << totalLength << "] \"DATA\" using 1:2 w l" << std::endl;
  std::cerr << "gnuplot> # Plot Phase Change" << std::endl;
  std::cerr << "gnuplot> plot [:" << totalLength << "] \"DATA\" using 1:3 w l" << std::endl;

  pfloat_t *forward1, *forward2, *inverse, *mute, *out1, *out2;
  forward1 = new pfloat_t[totalLength];
  forward2 = new pfloat_t[totalLength];
  mute = new pfloat_t[totalLength];
  UTILS::mute(mute, totalLength);
  inverse = new pfloat_t[totalLength];
  out1 = new pfloat_t[totalLength*2];
  out2 = new pfloat_t[totalLength*2];

  DELAY DD;
  long dsize = 24;
  DD.setsize(dsize);
  
  for(long l = 0;l < totalLength;l++)
    {
      pfloat_t input = S.process(1);
      pfloat_t output = input+0.9*DD.getlast();
      DD.process(output);
      forward1[l] = output;
      forward2[l] = input;
    }
  
  std::cerr << "Inverse Log Sweep Calculation..." << std::endl;
  IRMODEL3 FIR;
  S.setInverseMode(true); S.init();
  for(long l = 0;l < totalLength;l++){ inverse[l] = S.process(1); }
  FIR.setwetr(1); FIR.setdryr(0);
  FIR.loadImpulse(inverse,inverse,totalLength);
  
  std::cerr << "Inverse Log Sweep Convolution Calculation..." << std::endl;
  FIR.processreplace(forward1,forward2,out1,out2,totalLength,FV3_IR_MUTE_DRY|FV3_IR_SKIP_FILTER);
  FIR.processreplace(mute,mute,out1+totalLength,out2+totalLength,totalLength,FV3_IR_MUTE_DRY|FV3_IR_SKIP_FILTER);

  std::cerr << "Impulse FFT Calculation..." << std::endl;
  FFTW_(plan) p, q;
  p = FFTW_(plan_r2r_1d)(totalLength*2, out1, out1, FFTW_R2HC, FFTW_ESTIMATE);
  q = FFTW_(plan_r2r_1d)(totalLength*2, out2, out2, FFTW_R2HC, FFTW_ESTIMATE);
  FFTW_(execute)(p); FFTW_(execute)(q);
  FFTW_(destroy_plan)(p); FFTW_(destroy_plan)(q);

  forward1[0] = std::sqrt(out1[0]*out1[0]+out1[totalLength]*out1[totalLength]) /
    std::sqrt(out2[0]*out2[0]+out2[totalLength]*out2[totalLength]);
  forward2[0] = 0;
  for(long l = 1;l < (totalLength-1);l ++)
    {
      // amplitude
      forward1[l] =
	std::sqrt(out1[l]*out1[l]+out1[totalLength*2-l]*out1[totalLength*2-l]) /
	std::sqrt(out2[l]*out2[l]+out2[totalLength*2-l]*out2[totalLength*2-l]);
      // phase
      forward2[l] =
	std::atan2(out1[totalLength*2-l],out1[l]) - std::atan2(out2[totalLength*2-l],out2[l]);
      if(forward2[l] > M_PI)      forward2[l] -= 2*M_PI;
      if(forward2[l] < M_PI*(-1)) forward2[l] += 2*M_PI;
    }
  for(long l = 0;l < (totalLength-1);l ++)
    {
      printf("%ld %f %f\n", l, forward1[l], forward2[l]);
    }
  return 0;
}
