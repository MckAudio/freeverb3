/**
 *  Freeverb3 Impulse Response Processor sample
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

#include "fv3_config.h"

#include <cstdlib>
#include <cstring>
#include <cmath>

#include <stdint.h>

#include <sndfile.h>
#include <sndfile.hh>
#include <freeverb/irmodels.hpp>
#include <freeverb/irmodel1.hpp>
#include <freeverb/irmodel2.hpp>
#include <freeverb/irmodel3.hpp>
#include <freeverb/utils.hpp>
#include <fftw3.h>

#ifdef JACK
#include <jack/jack.h>
#endif

#include "CArg.hpp"

using namespace fv3;

#ifndef DIST_STRING
#define DIST_STRING ""
#endif

#ifdef PLUGDOUBLE
typedef fv3::irbase_ IRBASE;
typedef fv3::irmodel1_ IR1;
typedef fv3::irmodel2_ IR2;
typedef fv3::irmodel3_ IR3;
typedef fv3::irmodels_ IRS;
typedef fv3::utils_ UTILS;
typedef double pfloat_t;
#else
typedef fv3::irbase_f IRBASE;
typedef fv3::irmodel1_f IR1;
typedef fv3::irmodel2_f IR2;
typedef fv3::irmodel3_f IR3;
typedef fv3::irmodels_f IRS;
typedef fv3::utils_f UTILS;
typedef float pfloat_t;
#endif

unsigned options = FV3_IR_DEFAULT|FV3_IR_SKIP_FILTER;
CArg args;
IRBASE *ir;
SndfileHandle *input, *impulse;
int fragmentSize = 4096;
pfloat_t idb = -5, odb = -25;

void dump(void * v, int t)
{
  unsigned char * p = (unsigned char *)v;
  for(int i = 0;i < t;i ++)
    {
      std::fprintf(stdout, "%c", p[i]);
    }
}

void splitLR(pfloat_t * data, pfloat_t * L, pfloat_t * R,
	     int singleSize, int channels)
{
  for(int t = 0; t < singleSize; t++)
    {
      L[t] = data[t*channels+0];
      if(channels > 1)
	R[t] = data[t*channels+1];
      else
	R[t] = L[t];
   }
}

void mergeLR(pfloat_t * data, pfloat_t * L, pfloat_t * R,
	     int singleSize)
{
  for(int t = 0;t < singleSize;t ++)
    {
      data[t*2+0] = L[t];
      data[t*2+1] = R[t];
    }
}

void dumpLR(pfloat_t * l, pfloat_t * r, int t)
{
  pfloat_t * buf = new pfloat_t[2*t];
  mergeLR(buf,l,r,t);
  dump(buf, sizeof(pfloat_t)*t*2);
  delete[] buf;
}

void process(SndfileHandle * input, IRBASE * irm, int fsize)
{
  int count = 0;
  unsigned long acount = 0;
 pfloat_t *stream, *iL, *iR, *oL, *oR;
  stream = new pfloat_t[fsize*2];
  iL = new pfloat_t[fsize];
  iR = new pfloat_t[fsize];
  oL = new pfloat_t[fsize];
  oR = new pfloat_t[fsize];

  while(1)
    {
      count = input->readf(stream, fsize);
      if(count == 0) break;
      splitLR(stream, iL, iR, count, input->channels());
      irm->processreplace(iL,iR,oL,oR,fsize,options);
      dumpLR(oL,oR,fsize);
      acount += fsize;
      std::fprintf(stderr, "%016lx\r", acount);
    }

  UTILS::mute(iL, fsize);
  UTILS::mute(iR, fsize);
  for(long latency = irm->getLatency();
      latency >= 0; latency -= fsize)
    {
      count = fsize > latency ? latency : fsize;
      irm->processreplace(iL,iR,oL,oR,count,options);
      dumpLR(oL,oR,count);
      acount += count;
      std::fprintf(stderr, "%016lx\r", acount);
    }

  delete[] stream;
  delete[] iL;
  delete[] iR;
  delete[] oL;
  delete[] oR;
}

void help(const char * cmd)
{
  std::fprintf(stderr,
	       "Usage: %s [options] Input.wav ImpulseResponse.wav\n"
	       "Input, ImpulseReponse: libsndfile supported 2 channel file.\n"
	       "[[Options]]\n"
	       "-m irmodel type\n"
	       "\tn MODEL\n"
	       "\tdefault\n"
	       "\t2\n"
	       "\t0 irmodel2 fastest\n"
	       "\t1 irmodel  basic\n"
	       "\t3 irmodel3 zero latency\n"
	       "\t4 irmodels time base, too slow, only for testing\n"
	       "-f process fragmentSize\n"
	       "-indb Input Fader (arg-5)[dB]\n"
	       "-imdb Impulse Fader (arg-25)[dB]\n"
	       "[[Example]]\n"
	       "%s Input.wav IR.wav -imdb -5|aplay -f FLOAT_LE -c 2 -r 48000\n"
	       "\n",
	       cmd, cmd);
}

int main(int argc, char* argv[])
{
  std::fprintf(stderr, "Impulse Response Processor\n");
  std::fprintf(stderr, "<" PACKAGE DIST_STRING "-" VERSION ">\n");
  std::fprintf(stderr, "Copyright (C) 2007-2014 Teru Kamogashira\n");
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n", (int)sizeof(pfloat_t));

  if(argc <= 1) help(argv[0]), exit(-1);
  if(args.registerArg(argc, argv) != 0) exit(-1);
  
  input = new SndfileHandle(args.getFileArg(0));
  if(input->frames() == 0)
    {
      std::fprintf(stderr, "ERROR: open PCM file %s.\n",
		   args.getFileArg(0));
      exit(-1);
    }

  impulse = new SndfileHandle(args.getFileArg(1));
  if(impulse->frames() == 0)
    {
      std::fprintf(stderr, "ERROR: open PCM file %s.\n", args.getFileArg(1));
      exit(-1);
    }

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
    case 4:
      std::fprintf(stderr, "MODEL = irmodels\n");
      ir = new IRS();
      break;
    case 0:
    case 2:
    default:
      std::fprintf(stderr, "MODEL = irmodel2\n");
      ir = new IR2();
      break;
    }

  pfloat_t * irStream =
    new pfloat_t[((int)impulse->frames())*impulse->channels()];
  sf_count_t rcount = impulse->readf(irStream, impulse->frames());
  if(rcount != impulse->frames())
    {
      std::fprintf(stderr, "ERROR: readf impulse\n");
      delete[] irStream;
      exit(-1);
    }
  pfloat_t * irL = new pfloat_t[(int)impulse->frames()];
  pfloat_t * irR = new pfloat_t[(int)impulse->frames()];
  splitLR(irStream, irL, irR, impulse->frames(), impulse->channels());
  ir->loadImpulse(irL, irR, impulse->frames());
  std::fprintf(stderr, "Size = %ld, Latency = %ld\n", ir->getImpulseSize(), ir->getLatency());

  idb += args.getDouble("-indb");
  odb += args.getDouble("-imdb");
  std::fprintf(stderr, "Input %.1f[dB] Impulse %.1f[dB]\n", idb, odb);
  ir->setdry(idb);
  ir->setwet(odb);

  if((args.getLong("-f")) > 0) fragmentSize = args.getLong("-f");
  std::fprintf(stderr, "fragmentSize = %d\n", fragmentSize);
  std::fprintf(stderr, "\n");

  process(input, ir, fragmentSize);

  delete[] irL;
  delete[] irR;
  delete[] irStream;
  delete input;
  delete ir;
  std::fprintf(stderr, "\n");
  return 0;
}
