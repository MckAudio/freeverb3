/**
 *  Freeverb3 Impulse Response Processor Test Program
 *
 *  Copyright (C) 2007-2014 Teru Kamogashira
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
#include <freeverb/irmodel1.hpp>
#include <freeverb/irmodel2zl.hpp>
#include <freeverb/irmodel3.hpp>
#include <freeverb/utils.hpp>

#define GCC_IACA_START __asm__("movl $111, %ebx; .byte 0x64; .byte 0x67; .byte 0x90")
#define GCC_IACA_END   __asm__("movl $222, %ebx; .byte 0x64; .byte 0x67; .byte 0x90")

#ifdef BUILD_DOUBLE
typedef fv3::irmodel2zl_ IR2ZLD;
typedef fv3::utils_ UTILSD;
#endif

#ifdef BUILD_FLOAT
typedef fv3::irmodel2zl_f IR2ZLF;
typedef fv3::utils_f UTILSF;
#endif

#define N 2048
#define IN 4096

template <typename pfloat_t, typename IR2, typename UTILS>
class Test
{
public:
  void show()
  {
    fprintf(stderr, "========\n");
    fprintf(stderr, "X87CW:     0x%04x\n", UTILS::getX87CW());
    fprintf(stderr, "SIMDFlag:  0x%08x\n", UTILS::getSIMDFlag());
    fprintf(stderr, "MXCSR:     0x%08x\n", UTILS::getMXCSR());
    fprintf(stderr, "MXCSRMASK: 0x%08x\n", UTILS::getMXCSR_MASK()); 
  }
  
  void test(uint32_t flag1, uint32_t flag2)
  {
    fprintf(stderr, "--------\n");
    
    IR2 IR;
    pfloat_t S[N], I1[IN], I2[IN], O1[IN], O2[IN];
    
    for(long i = 0;i < N;i ++) S[i] = i+1;
    IR.setSIMD(flag1,flag2);
    IR.loadImpulse(S, S, N);
    if(flag1 != IR.getSIMD(0)) fprintf(stderr, "Not implemented or supported.\n");
    fprintf(stderr, "SIMDFlag(%08x,%08x)->%08x,%08x ", flag1, flag2, IR.getSIMD(0), IR.getSIMD(1));
    
    UTILS::mute(I1, IN); UTILS::mute(I2, IN);
    UTILS::mute(O1, IN); UTILS::mute(O2, IN);
    I1[0] = 1;
    I2[1] = 1; // increment
    
    IR.setdryr(0); IR.setwetr(1);
    IR.processreplace(I1, I2, O1, O2, IN, FV3_IR_MUTE_DRY|FV3_IR_SKIP_FILTER);
    
    pfloat_t diff = 0;
    for(int i = 0;i < N;i ++)
      {
        pfloat_t df = std::abs(O1[i] - (pfloat_t)(i+1L));
        if(df > diff) diff = df;
      }
    for(int i = 0;i < N;i ++)
      {
        pfloat_t df = std::abs(O2[i] - (pfloat_t)i);
        if(df > diff) diff = df;
      }
    fprintf(stderr, "Max. Error = %.15f ", (double)diff);
    for(int i = 0;i < 4;i ++) fprintf(stderr, " %1.1f/%1.1f", (double)O1[i], (double)O2[i]);
    fprintf(stderr, "\n");
  }
};

int main()
{
  fprintf(stderr, "Impulse Response Processor Test\n");
  fprintf(stderr, "<" PACKAGE "-" VERSION ">\n");
  fprintf(stderr, "Copyright (C) 2006-2018 Teru Kamogashira\n");

  fprintf(stderr, "IR length: %d\n", N);

#ifdef BUILD_FLOAT
  Test<float,IR2ZLF,UTILSF> testf;
  fprintf(stderr, "========\n");
  fprintf(stderr, "Single Precision\n");
  testf.show();
  testf.test(FV3_X86SIMD_FLAG_FPU,0);
  testf.test(FV3_X86SIMD_FLAG_3DNOWP,0);
  testf.test(FV3_X86SIMD_FLAG_SSE,0);
  testf.test(FV3_X86SIMD_FLAG_SSE,FV3_X86SIMD_FLAG_SSE_V1);
  testf.test(FV3_X86SIMD_FLAG_SSE3,0);
  testf.test(FV3_X86SIMD_FLAG_AVX,0);
  testf.test(FV3_X86SIMD_FLAG_FMA3,0);
  testf.test(FV3_X86SIMD_FLAG_FMA4,0);
  testf.test(FV3_X86SIMD_FLAG_SSE2,0); // not implemented test; default to FPU
#endif
  
#ifdef BUILD_DOUBLE
  Test<double,IR2ZLD,UTILSD> testd;
  fprintf(stderr, "========\n");
  fprintf(stderr, "Double Precision\n");
  testd.show();
  testd.test(FV3_X86SIMD_FLAG_FPU,0);
  testd.test(FV3_X86SIMD_FLAG_SSE2,0);
  testd.test(FV3_X86SIMD_FLAG_SSE4_1,0);
  testd.test(FV3_X86SIMD_FLAG_AVX,0);
  testd.test(FV3_X86SIMD_FLAG_FMA3,0);
  testd.test(FV3_X86SIMD_FLAG_FMA4,0);
#endif

#ifdef BUILD_FLOAT
  fprintf(stderr, "========\n");
  fprintf(stderr, "MXCSR Single Precision\n");
  UTILSF::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  fprintf(stderr, "MXCSR:     0x%08x\n", UTILSF::getMXCSR());
  testf.test(FV3_X86SIMD_FLAG_SSE,0);
#endif
#ifdef BUILD_DOUBLE
  fprintf(stderr, "========\n");
  fprintf(stderr, "MXCSR Double Precision\n");
  UTILSD::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  fprintf(stderr, "MXCSR:     0x%08x\n", UTILSD::getMXCSR());
  testd.test(FV3_X86SIMD_FLAG_SSE2,0);
#endif  
}
