/**
 *  Freeverb3 Benchmark Program
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

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>

#include <sndfile.h>
#include <freeverb/irmodels.hpp>
#include <freeverb/irmodel1.hpp>
#include <freeverb/irmodel2.hpp>
#include <freeverb/irmodel2zl.hpp>
#include <freeverb/irmodel3.hpp>
#ifdef ENABLE_PTHREAD
#include <freeverb/irmodel3p.hpp>
#endif
#include <fftw3.h>

#include "CArg.hpp"

#ifdef PLUGDOUBLE
typedef fv3::irbase_ IRBASE;
typedef fv3::irmodels_ IRS;
typedef fv3::irmodel1_ IR1;
typedef fv3::irmodel2_ IR2;
typedef fv3::irmodel2zl_ IR2ZL;
typedef fv3::irmodel3_ IR3;
#ifdef ENABLE_PTHREAD
typedef fv3::irmodel3p_ IR3P;
#endif
typedef fv3::utils_ UTILS;
typedef double pfloat_t;
#else
typedef fv3::irbase_f IRBASE;
typedef fv3::irmodels_f IRS;
typedef fv3::irmodel1_f IR1;
typedef fv3::irmodel2_f IR2;
typedef fv3::irmodel2zl_f IR2ZL;
typedef fv3::irmodel3_f IR3;
#ifdef ENABLE_PTHREAD
typedef fv3::irmodel3p_f IR3P;
#endif
typedef fv3::utils_f UTILS;
typedef float pfloat_t;
#endif

CArg args;
IRBASE *ir;

long impulseLength = 48000*10;
long fragmentSize = 1024;
long factor = 16;

long processFrame = 1024;
long frameCount = 1000;

static double loadIR(IRBASE *irm, long size, long fsize, long factor)
{
  clock_t time_start = 0, time_end = 0;
  double time_diff;
  pfloat_t * irL = new pfloat_t[size]; UTILS::mute(irL, size);
  pfloat_t * irR = new pfloat_t[size]; UTILS::mute(irR, size);
  
  IR2 *ir2 = dynamic_cast<IR2*>(irm);
  IR3 *ir3 = dynamic_cast<IR3*>(irm);
  if(ir2 != NULL)
    {
      std::fprintf(stderr, "IR2\n");
      time_start = clock();
      ir2->setFragmentSize(fsize*factor);
      ir2->loadImpulse(irL,irR,size);
      time_end = clock();
    }
  else if(ir3 != NULL)
    {
      std::fprintf(stderr, "IR3\n");
      time_start = clock();
      ir3->setFragmentSize(fsize,factor);
      ir3->loadImpulse(irL,irR,size);
      time_end = clock();
    }
  else
    {
      time_start = clock();
      irm->loadImpulse(irL,irR,size);
      time_end = clock();
    }

  time_diff = (double)(time_end-time_start)/CLOCKS_PER_SEC;
  delete[] irL;
  delete[] irR;
  return time_diff;
}

static double process(IRBASE *irm, long fsize, long count, unsigned options)
{
  clock_t time_start = 0, time_end = 0;
  double time_diff;
  pfloat_t * iL = new pfloat_t[fsize]; UTILS::mute(iL, fsize); 
  pfloat_t * iR = new pfloat_t[fsize]; UTILS::mute(iR, fsize);
  pfloat_t * oL = new pfloat_t[fsize]; UTILS::mute(oL, fsize);
  pfloat_t * oR = new pfloat_t[fsize]; UTILS::mute(oR, fsize);
  
  time_start = clock();
  for(int i = 0;i < count;i ++)
    {
      irm->processreplace(iL,iR,oL,oR,fsize,options);
    }
  time_end = clock();

  time_diff = (double)(time_end-time_start)/CLOCKS_PER_SEC;  
  delete[] iL; delete[] iR; delete[] oL; delete[] oR;
  return time_diff;
}

static void help(const char * cmd)
{
  std::fprintf(stderr,
               "Usage: %s [options]\n"
               "[[Options]]\n"
               "-m irmodel type\n"
               "\tn MODEL\n"
               "\t0=2 irmodel2 (default)\n"
               "\t1 irmodel1   basic\n"
               "\t2 irmodel2   fastest\n"
               "\t5 irmodel2zl zero latency\n"
               "\t3 irmodel3   zero latency\n"
               "\t4 irmodels   time base, too slow, only for testing\n"
#ifdef ENABLE_PTHREAD
               "\t6 irmodel3p  zero latency pthread\n"
#endif
               "-ir impulseLength (480000)\n"
               "-fr fragmentSize (1024)\n"
               "-fa factor (16)\n"
               "-pf processFrame (1024)\n"
               "-fc frameCount (1000)\n"
               "\n",
               cmd);
}

int main(int argc, char * argv[])
{
  std::fprintf(stderr, "Impulse Response Processor Benckmark\n");
  std::fprintf(stderr, "<" PACKAGE "-" VERSION ">\n");
  std::fprintf(stderr, "Copyright (C) 2006-2014 Teru Kamogashira\n");
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n", (int)sizeof(pfloat_t));
  
  if(argc <= 1) help(argv[0]);
  if(args.registerArg(argc, argv) != 0) exit(-1);
  long model = args.getLong("-m");
  switch(model)
    {
    case 1:
      std::fprintf(stderr, "MODEL = irmodel1\n");
      ir = new IR1();
      break;
    case 3:
      std::fprintf(stderr, "MODEL = irmodel3\n");
      ir = new IR3();
      break;
#ifdef ENABLE_PTHREAD
    case 6:
      std::fprintf(stderr, "MODEL = irmodel3p\n");
      ir = new IR3P();
      break;
#endif
    case 4:
      std::fprintf(stderr, "MODEL = irmodels\n");
      ir = new IRS();
      break;
    case 5:
      std::fprintf(stderr, "MODEL = irmodel2zl\n");
      ir = new IR2ZL();
      break;
    case 0:
    case 2:
    default:
      std::fprintf(stderr, "MODEL = irmodel2\n");
      ir = new IR2();
      break;
    }
  if(args.getLong("-ir") > 0) impulseLength = args.getLong("-ir");
  if(args.getLong("-fr") > 0) fragmentSize = args.getLong("-fr");
  if(args.getLong("-fa") > 0) factor = args.getLong("-fa");

  if(args.getLong("-pf") > 0) processFrame = args.getLong("-pf");
  if(args.getLong("-fc") > 0) frameCount = args.getLong("-fc");

  std::fprintf(stderr, "loadIR( %ld %ld %ld )\n", impulseLength, fragmentSize, factor);
  double ltime = loadIR(ir, impulseLength, fragmentSize, factor);
  std::fprintf(stderr, "process( %ld x %ld )\n", processFrame, frameCount);
  double ptime = process(ir, processFrame, frameCount, FV3_IR_SKIP_FILTER);
  std::fprintf(stderr, "\n");

  std::fprintf(stderr, "-=-=-=-=- Results -=-=-=-=-\n");
  std::fprintf(stderr, "irLength  %ld (%.2f[s]@48kHz)\n",
	       impulseLength, (double)impulseLength/48000);
  std::fprintf(stderr, "fragmentS %ld\n", fragmentSize);
  std::fprintf(stderr, "factor    %ld\n", factor);
  std::fprintf(stderr, "process t %ld (%.2f[s]@48kHz)\n",
	       processFrame*frameCount, (double)processFrame*frameCount/48000);
  std::fprintf(stderr, "load time %.2f[s] (x%.2f@48kHz)\n",
	       ltime, (double)impulseLength/48000/ltime);
  std::fprintf(stderr, "process t %.2f[s] (x%.2f@48kHz)\n",
	       ptime, (double)processFrame*frameCount/48000/ptime);
  std::fprintf(stderr, "\n");
  return 0;
}
