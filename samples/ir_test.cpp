/**
 *  Freeverb3 Impulse Response Processor Test Program
 *
 *  Copyright (C) 2007-2011 Teru Kamogashira
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

#include <cmath>
#include <freeverb/irmodel3.hpp>
#include <freeverb/utils.hpp>

#define GCC_IACA_START __asm__("movl $111, %ebx; .byte 0x64; .byte 0x67; .byte 0x90")
#define GCC_IACA_END   __asm__("movl $222, %ebx; .byte 0x64; .byte 0x67; .byte 0x90")

#ifdef PLUGDOUBLE
typedef fv3::irmodel3_ IR3;
typedef fv3::utils_ UTILS;
typedef double pfloat_t;
#else
typedef fv3::irmodel3_f IR3;
typedef fv3::utils_f UTILS;
typedef float pfloat_t;
#endif

#define N 2048
#define IN 4096
IR3  IR;
pfloat_t S[N], I1[IN], I2[IN], O1[IN], O2[IN];

int main()
{
  fprintf(stdout, "SIMDFlag:  0x%08x\n", UTILS::getSIMDFlag());
  fprintf(stdout, "X87CW:     0x%04x\n", UTILS::getX87CW());
  fprintf(stdout, "MXCSR:     0x%08x\n", UTILS::getMXCSR());
  fprintf(stdout, "MXCSRMASK: 0x%08x\n", UTILS::getMXCSR_MASK());
  for(long i = 0;i < N;i ++) S[i] = i+1;
  UTILS::mute(I1, IN); UTILS::mute(I2, IN);
  UTILS::mute(O1, IN); UTILS::mute(O2, IN);
  I1[0] = 1; I2[1] = 1;

  IR.loadImpulse(S, S, N);
  IR.setdryr(0); IR.setwetr(1);
  IR.processreplace(I1, I2, O1, O2, IN, FV3_IR_MUTE_DRY|FV3_IR_SKIP_FILTER);

  pfloat_t diff = 0;
  for(int i = 0;i < N;i ++)
    {
      pfloat_t df = std::abs(O1[i] - (pfloat_t)(i+1l));
      if(df > diff) diff = df;
    }
  for(int i = 0;i < N;i ++)
    {
      pfloat_t df = std::abs(O2[i] - (pfloat_t)i);
      if(df > diff) diff = df;
    }
#ifdef PLUGDOUBLE
  fprintf(stdout, "Max Error Diff = %.15f\n", diff);
#else
  fprintf(stdout, "Max Error Diff = %.7f\n", diff);
#endif
  fprintf(stdout, "MXCSR:     0x%08x\n", UTILS::getMXCSR());
}
