/**
 *  Freeverb3 MLS generator
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
#include <freeverb/mls.hpp>

#include "CArg.hpp"

#ifdef PLUGDOUBLE
typedef fv3::lfsr_ MLS;
typedef fv3::utils_ UTILS;
typedef double pfloat_t;
#else
typedef fv3::lfsr_f MLS;
typedef fv3::utils_f UTILS;
typedef float pfloat_t;
#endif

CArg args;

long bitOrder = 16;

static void help(const char * cmd)
{
  std::fprintf(stderr,
	       "Usage: %s [options]\n"
	       "[[Options]]\n"
	       "-b bit order (16)\n"
	       "\n",
	       cmd);
}

int main(int argc, char * argv[])
{
  std::fprintf(stderr, "LFSR MLS generator\n");
  std::fprintf(stderr, "<" PACKAGE "-" VERSION ">\n");
  std::fprintf(stderr, "Copyright (C) 2006-2014 Teru Kamogashira\n");
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n", (int)sizeof(pfloat_t));
  
  if(argc <= 1) help(argv[0]);
  if(args.registerArg(argc, argv) != 0)
    {
      help(argv[0]);
      exit(-1);
    }

  if(args.getLong("-b") > 0)
    bitOrder = args.getLong("-b");
  if(bitOrder > 24)
    {
      std::fprintf(stderr, "Max Bit Order : 24\n");
      bitOrder = 24;
    }
  std::fprintf(stderr, "LFSR Bit Order : %ld\n", bitOrder);

  long size = (1<<bitOrder)/sizeof(uint32_t)/8;
  std::fprintf(stderr, "uint32_t alloc size : %ld (MLS size/sizeof(uint32_t)/8)\n", size);
  uint32_t * data = new uint32_t[size];
  MLS mls;
  mls.setBitSize(bitOrder);

  std::fprintf(stderr, "Generating...");
  mls.mls(data, size);
  std::fprintf(stderr, "done.\n");

  std::fprintf(stderr, "uint32_t to float conversion... UInt32ToFloat(uint32,float,sizeOfUint32)\n");
  pfloat_t * forward1 = new pfloat_t[1<<bitOrder];
  mls.UInt32ToFloat(data,forward1,size);

  for(long i = 0;i < (1<<bitOrder)-1;i ++)
    {
      std::fprintf(stdout, "%f\n", forward1[i]);
    }
  
  return 0;
}
