/**
 *  Freeverb3 Sampling Rate Converter
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <stdint.h>

#include <sndfile.h>
#include <sndfile.hh>
#include <fftw3.h>
#include <freeverb/irmodel2.hpp>
#include <gdither.h>

#include "CArg.hpp"

#define DEFAULT_TARGET_FS 48000
#define DEFAULT_MODE W_KAISER
#define DEFAULT_KAISER_PARAMETER 140
#define DEFAULT_COSRO_PARAMETER 0.1
#define DEFAULT_TRANSITION_BAND 1

#ifndef DIST_STRING
#define DIST_STRING ""
#endif

#ifdef PLUGDOUBLE
typedef fv3::irbase_ IRBASE;
typedef fv3::irmodel2_ IR2;
typedef double pfloat_t;
#else
typedef fv3::irbase_f IRBASE;
typedef fv3::irmodel2_f IR2;
typedef float pfloat_t;
#endif

static IR2 reverbm;
const unsigned options = FV3_IR_DEFAULT|FV3_IR_SKIP_FILTER|FV3_IR_MUTE_DRY;
//const long fragmentSize = 512;
//const long ffactor = 32;
long groupDelay = 0;
const long VST_frame = 16384;
pfloat_t clipL = 0.0, clipR = 0.0;
pfloat_t attenuation = 1.0f;

CArg args;

SndfileHandle * output;
static long outputFormat = SF_FORMAT_WAV|SF_FORMAT_FLOAT;
static long outputMode = 0;

#define OMODE_FL 0
#define OMODE_16 1
#define OMODE_24 2
#define OMODE_32 3

static GDither pdither;
static long ditherMode = GDitherNone;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

static long checkFileExist(const char * filename)
{
  FILE * fp = fopen(filename ,"rb");
  if(fp == NULL)
    return 0;
  return 1;
}

static long gcmi(long a, long b)
{
  if(a < b) return gcmi(b, a);
  long r = a % b;
  if(r == 0) return b;
  else return gcmi(b, r);
}

static long lcmi(long a, long b)
{
  long g = gcmi(a, b);
  return (a/g) * (b/g) * g;
}

// FIR FILTER C

// Windows

static void Square(pfloat_t w[], const long N)
{
  long i;
  for(i = 0;i < N;i ++)
    {
      w[i] = 1;
    }
  return;
}

static void Hamming(pfloat_t w[], const long N)
{
  long i;
  const pfloat_t M = N-1;  
  for(i = 0; i < N; i++)
    {
      w[i] = 0.54 - (0.46*cos(2.0*M_PI*(pfloat_t)i/M));
    }
  return;
}

static void Hanning(pfloat_t w[], const long N)
{
  long i;
  const pfloat_t M = N-1;
  for(i = 0; i < N; i++)
    {
      w[i] = 0.5 * (1.0 - cos(2.0*M_PI*(pfloat_t)i/M));
    }
  return;
}

static void Blackman(pfloat_t w[], const long N)
{
  long i;
  const pfloat_t M = N-1;
  for(i = 0; i < N; i++)
    {
      w[i] =
	0.42 - (0.5 * cos(2.0*M_PI*(pfloat_t)i/M)) +
	(0.08*cos(4.0*M_PI*(pfloat_t)i/M));
    }
  return;
}

#ifndef isfinite
#define isfinite(v) std::isfinite(v)
#endif

static pfloat_t i_zero(pfloat_t x)
{
  /*
    The zeroth order Modified Bessel function:
    
    I_0(x) = 1 + 
    x^2 / 2^2 + 
    x^4 / (2^2 * 4^2) +
    x^6 / (2^2 * 4^2 * 6^2) +
    ... to infinity        
    
    This function grows quite quickly towards very large numbers.
    By re-organising the calculations somewhat we minimise the dynamic
    range in the floating polong numbers, and can thus calculate the
    function for larger x than we could do with a naive implementation.  
  */
  pfloat_t n, a, halfx, sum;
  halfx = x / 2.0;
  sum = 1.0;
  a = 1.0;
  n = 1.0;
  do {
    a *= halfx;
    a /= n;
    sum += a * a;
    n += 1.0;
    /* either 'sum' will reach +inf or 'a' zero... */
  } while (a != 0.0 && isfinite(sum));
  return sum;
}

static void Kaiser(pfloat_t w[], const long N, pfloat_t sl)
{
  long i;
  pfloat_t beta = 0.5;
  // -sl[dB]
  if(sl > 50)
    beta = 0.1102*(sl-8.7);
  else if(sl >= 20)
    beta = 0.5842 * std::pow((double)sl-21, 0.4) + 0.07886*(sl-21);
  else
    beta = 0;
  std::fprintf(stderr, "Kaiser beta=%f\n", beta);
  const pfloat_t M = N-1;
  pfloat_t inv_izbeta = 1.0 / i_zero(M_PI*beta);
  for(i = 0;i < N;i ++)
    {
      w[i] =
	i_zero(M_PI*beta*sqrt(1-std::pow(2*(pfloat_t)i/M-1, 2))) * inv_izbeta;
    }
}

static void CosROW(pfloat_t w[], const long &N, const pfloat_t &fc,
		   const pfloat_t &alpha)
{
  long i;
  const pfloat_t M = N-1;
  pfloat_t n;
  for(i = 0; i < N; i++)
    {
      n = (pfloat_t)i - M/2.0;
      w[i] = std::cos(M_PI*n*2.0*fc*M_PI*alpha)
	/ (1 - std::pow((M_PI*n*2.0*fc*2*alpha),2));
    }        
  return;
}

// Sinc

static void Sinc(pfloat_t sinc[], const long N, const pfloat_t &fc)
{
  long i;
  const pfloat_t M = N-1;
  pfloat_t n;
  // Generate sinc delayed by (N-1)/2
  for(i = 0; i < N; i++)
    {
      if(i == M/2.0)
	{
	  sinc[i] = 2.0 * fc;
	}
      else
	{
	  n = (pfloat_t)i - M/2.0;
	  sinc[i] = std::sin(M_PI*n*2.0*fc) / (M_PI*n);
	}
    }        
  return;
}

const long W_BLACKMAN = 1;
const long W_HANNING = 2;
const long W_HAMMING = 3;
const long W_KAISER = 4;
const long W_COSRO = 5;
const long W_SQUARE = 6;

// LPF

static void firLPF(pfloat_t h[], const long N,
		   const long WINDOW, const pfloat_t fc,
		   const pfloat_t sl, const pfloat_t alpha)
{
  long i;
  pfloat_t * w = new pfloat_t[N];
  pfloat_t * sinc = new pfloat_t[N];
  Sinc(sinc, N, fc);
  switch(WINDOW)
    {
    case W_SQUARE:
      std::fprintf(stderr, "W_SQUARE\n");
      Square(w, N);
      break;
    case W_BLACKMAN:
      std::fprintf(stderr, "W_BLACKMAN\n");
      Blackman(w, N);
      break;
    case W_HANNING:
      std::fprintf(stderr, "W_HANNING\n");
      Hanning(w, N);
      break;
    case W_HAMMING:
      std::fprintf(stderr, "W_HAMMING\n");
      Hamming(w, N);
      break;
    case W_KAISER:
      std::fprintf(stderr, "W_KAISER -%fdB\n", sl);
      Kaiser(w, N, sl);
      break;
    case W_COSRO:
      std::fprintf(stderr, "W_COSRO alpha=%f\n", alpha);
      CosROW(w, N, fc, alpha);
      break;
    default:
      std::fprintf(stderr, "Invalid window mode: %ld\n", WINDOW);
      std::exit(-1);
      break;
    }
  for(i = 0; i < N; i++)
    {
      h[i] = sinc[i] * w[i];
    }
  delete[] w;
  delete[] sinc;
  return;
}

#if 0
static void firHPF(pfloat_t h[], const long N,
	    const long WINDOW, const pfloat_t fc,
	    const pfloat_t sl, const pfloat_t alpha)
{
  long i;
  firLPF(h, N, WINDOW, fc, sl, alpha);
  // 2. Spectrally invert (negate all samples and add 1 to center sample) lowpass filter
  // = delta[n-((N-1)/2)] - h[n], in the time domain
  for(i = 0;i < N;i ++)
    {
      h[i] *= -1.0;	// = 0 - h[i]
    }
  h[(N-1)/2] += 1.0;	// = 1 - h[(N-1)/2]
  return;
}

static void firBSF(double h[], const long N,
	    const long WINDOW, const pfloat_t fc1, const pfloat_t fc2,
	    const pfloat_t sl, const pfloat_t alpha)
{
  long i;
  pfloat_t *h1 = new pfloat_t[N];
  pfloat_t *h2 = new pfloat_t[N];
  firLPF(h1, N, WINDOW, fc1, sl, alpha);
  firHPF(h2, N, WINDOW, fc2, sl, alpha);
  for(i = 0;i < N;i ++)
    {
      h[i] = h1[i] + h2[i];
    }
  delete[] h1;
  delete[] h2;
  return;
}

static void firBPF(double h[], const int N,
	     const int WINDOW, const double fc1, const double fc2,
	     const pfloat_t sl, const pfloat_t alpha)
{
  int i;
  firBSF(h, N, WINDOW, fc1, fc2, sl, alpha);
  for (i = 0; i < N; i++)
    {
      h[i] *= -1.0;	// = 0 - h[i]
    }
  h[(N-1)/2] += 1.0;	// = 1 - h[(N-1)/2]
  return;
}
#endif

// FIR FILTER C

static void setLPF(IRBASE * model, long len, pfloat_t fc,
		   const long WINDOW, const pfloat_t sl, const pfloat_t alpha)
{
  std::fprintf(stderr, "LPF: filterLength %ld limit %f\n", len, fc);
  pfloat_t * fir = new pfloat_t[len];
  firLPF(fir, len, WINDOW, fc, sl, alpha);
  //model->loadImpulse(fir, fir, len, FFTW_ESTIMATE, fragmentSize, ffactor);
  model->loadImpulse(fir, fir, len);
  delete[] fir;
}

static long predictN(long mode, pfloat_t transitionF,
		     const pfloat_t attenuation, const pfloat_t alpha)
{
  std::fprintf(stderr, "== predict filter length ===\n");
  std::fprintf(stderr, "Transition Bandwidth %f\n", transitionF);
  long N = 0;
  switch(mode)
    {
    case W_SQUARE:
      N = (long)std::ceil((1.8*M_PI)/(transitionF*M_PI))+1;
      break;
    case W_BLACKMAN:
      N = (long)std::ceil((11*M_PI)/(transitionF*M_PI))+1;
      break;
    case W_HANNING:
      N = (long)std::ceil((6.2*M_PI)/(transitionF*M_PI))+1;
      break;
    case W_HAMMING:
      N = (long)std::ceil((6.6*M_PI)/(transitionF*M_PI))+1;
      break;
    case W_KAISER:
      if(attenuation <= 21)
	N = (long)std::ceil(0.9222/transitionF)+1;
      else
	N = (long)std::ceil((attenuation-7.95)/(14.36*transitionF))+1;
      break;
    case W_COSRO:
      N = 0;
      break;
    default:
      std::fprintf(stderr, "Invalid window mode: %ld\n", mode);
      N = 0;
      break;
    }
  return N;
}

static void mute(pfloat_t * f, long t)
{
  for(long i = 0;i < t;i ++)
    f[i] = 0.0f;
}

static long rcount = 0;
static void readPad(pfloat_t * L, pfloat_t * R, long factor,
		    long size, SndfileHandle * snd)
{
  pfloat_t v[2];
  for(long i = 0;i < size;i ++)
    {
      if(rcount == 0)
	{
	  snd->readf(v, 1);
	  L[i] = v[0];
	  R[i] = v[1];
	}
      else
	{
	  L[i] = R[i] = 0;
	}
      rcount ++;
      if(rcount == factor)
	rcount = 0;
    }
}

static long wcount = 0;
static void writePick(pfloat_t * L, pfloat_t * R,
		      long factor, long size, double dfactor)
{
  float v[2];
  for(long i = 0;i < size;i ++)
    {
      if(wcount == 0)
	{
	  v[0] = dfactor*L[i]*attenuation;
	  v[1] = dfactor*R[i]*attenuation;
	  if(v[0] > 1.0||v[0] < -1.0)
	    {
	      clipL = v[0];
	    }
	  if(v[1] > 1.0||v[1] < -1.0)
	    {
	      clipR = v[1];
	    }
	  if(groupDelay > 0)
	    {
	      groupDelay --;
	    }
	  else
	    {
	      switch(outputMode)
		{
		case OMODE_16:
		  int16_t o16[2];
		  gdither_runf(pdither, 0, 1, v, o16);
		  gdither_runf(pdither, 1, 1, v, o16);
		  output->writef(o16, 1);
		  break;
		case OMODE_24:
		case OMODE_32:
		  int32_t o32[2];
		  gdither_runf(pdither, 0, 1, v, o32);
		  gdither_runf(pdither, 1, 1, v, o32);
		  output->writef(o32, 1);
		  break;
		default:
		  output->writef(v, 1);
		  break;
		}
	    }
	}
      wcount ++;
      if(wcount == factor)
	wcount = 0;
    }
}

static void processAll(IRBASE * model, SndfileHandle * snd,
		      long fBlock, long upBlock, long toBlock)
{
  long upFactor = upBlock/fBlock;
  double factor = (double)upFactor;
  long downFactor = upBlock/toBlock;
  long long upFrames = ((long long)upFactor)*snd->frames();
  std::fprintf(stderr, "Original samples: %lld\n", (long long)snd->frames());
  std::fprintf(stderr, "Total samples: %lld\n", upFrames);
  long count = (long)(upFrames/((long long)VST_frame));
  long rest  = (long)(upFrames%((long long)VST_frame));
  pfloat_t *iL, *iR, *oL, *oR;
  try
    {
      iL = new pfloat_t[VST_frame];
      iR = new pfloat_t[VST_frame];
      oL = new pfloat_t[VST_frame];
      oR = new pfloat_t[VST_frame];
    }
  catch(std::bad_alloc)
    {
      std::fprintf(stderr, "std::bad_alloc %ld x 4\n", VST_frame);
      std::exit(-1);
    }
  for(long i = 0;i < count;i ++)
    {
      std::fprintf(stderr, "%ld/%ld x %ld\r", i, count, VST_frame);
      readPad(iL, iR, upFactor, VST_frame, snd);
      model->processreplace(iL,iR,oL,oR,VST_frame,options);
      writePick(oL, oR, downFactor, VST_frame, factor);
    }
  std::fprintf(stderr, "\nrest = %ld", rest);
  readPad(iL, iR, upFactor, rest, snd);
  model->processreplace(iL,iR,oL,oR,rest,options);
  writePick(oL, oR, downFactor, rest, factor);

  std::fprintf(stderr, "\nWait for IR...\n");
  int wait = ((model->getImpulseSize()-1)/2) + model->getLatency();
  count = wait/VST_frame;
  rest  = wait%VST_frame;
  for(long i = 0;i < count;i ++)
    {
      std::fprintf(stderr, "%ld/%ld x %ld\r", i, count, VST_frame);
      mute(iL, VST_frame);
      mute(iR, VST_frame);
      model->processreplace(iL,iR,oL,oR,VST_frame,options);
      writePick(oL, oR, downFactor, VST_frame, factor);
    }
  std::fprintf(stderr, "\nrest = %ld", rest);
  mute(iL, rest);
  mute(iR, rest);
  model->processreplace(iL,iR,oL,oR,rest,options);
  writePick(oL, oR, downFactor, rest, factor);

  std::fprintf(stderr, "\ndone.\n");
  delete[] iL;
  delete[] iR;
  delete[] oL;
  delete[] oR;
}

void help(const char * cmd)
{
  std::fprintf(stderr, "%s [options] input output\n", cmd);
  std::fprintf(stderr, "Input: libsndfile supported file.\n");
  std::fprintf(stderr, "Output: WAV\n");
  std::fprintf(stderr, "[[Options]]\n");
  std::fprintf(stderr, "-r Target sampling rate\n");
  std::fprintf(stderr, "-m Mode\n");
  std::fprintf(stderr, "\tn Window        [Stopband Attenuation]\n");
  std::fprintf(stderr, "\t1 Blackman      [-74dB]\n");
  std::fprintf(stderr, "\t2 Hanning       [-44dB]\n");
  std::fprintf(stderr, "\t3 Hamming       [-53dB]\n");
  std::fprintf(stderr, "\t4 Kaiser        [-<parameter>dB]\n");
  std::fprintf(stderr, "\t5 CosineRollOff (alpha=<parameter>)\n");
  std::fprintf(stderr, "\t\t(alpha >0 && <=1) 0->default,\n");
  std::fprintf(stderr, "\t\tuse Square window when alpha=0.\n");
  std::fprintf(stderr, "\t6 Square        [-21dB]\n");
  std::fprintf(stderr, "-k Kaiser Parameter\n");
  std::fprintf(stderr, "-a CosineRollOff Parameter\n");
  std::fprintf(stderr, "-t Transition bandwidth [Hz]\n");
  std::fprintf(stderr, "\tignored if the window mode is CosineRollOff\n");
  std::fprintf(stderr, "-ow overwrite\n");
  std::fprintf(stderr, "\t1 = allow overwrite\n");
  std::fprintf(stderr, "-att Overall attenuation\n");
  std::fprintf(stderr, "-n Override filter length\n");
  std::fprintf(stderr, "-f output Precision\n");
  std::fprintf(stderr, "\t0 32bit Float Little Endian (default)\n");
  std::fprintf(stderr, "\t1 Signed 16 bit Little Endian (CD precision)\n");
  std::fprintf(stderr, "\t2 Signed 24 bit Little Endian in 3bytes\n");
  std::fprintf(stderr, "\t3 Signed 32 bit Little Endian\n");
  std::fprintf(stderr, "-gd gdither mode\n");
  std::fprintf(stderr, "\t0 None\n");
  std::fprintf(stderr, "\t1 Rect\n");
  std::fprintf(stderr, "\t2 Tri\n");
  std::fprintf(stderr, "\t3 Shaped\n");
  std::fprintf(stderr, "[[Example]]\n");
  std::fprintf(stderr, "1. conversion to 96k float\n");
  std::fprintf(stderr, "%s -r 96000 -m 4 -k 140 input.wav output.wav\n",
	  cmd);
  std::fprintf(stderr, "2. conversion to CD format with shaped dithering\n");
  std::fprintf(stderr, "%s -r 44100 -m 4 -k 140 -f 1 -gd 3 input.wav output.wav\n",
	  cmd);
  std::fprintf(stderr, "[[Note]]\n");
  std::fprintf(stderr, "If the conversion factor is not an integer,\n");
  std::fprintf(stderr, "it will take a lot to convert. (ex. 48k->44.1k)\n");
  std::fprintf(stderr, "\n");
}

int main(int argc, char* argv[])
{
  std::fprintf(stderr, "1-pass Sampling Rate Converter\n");
  std::fprintf(stderr, "<" PACKAGE DIST_STRING "-" VERSION ">\n");
  std::fprintf(stderr, "Copyright (C) 2007-2014 Teru Kamogashira\n");
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n\n", (int)sizeof(pfloat_t));
  
  if (argc <= 1)
    {
      help(argv[0]);
      exit(-1);
    }

  if(args.registerArg(argc, argv) != 0)
    exit(-1);

  long outputMode = args.getLong("-f");
  std::fprintf(stderr, "> outputMode = %ld\n", outputMode);
  switch(outputMode)
    {
    case OMODE_16:
      outputFormat = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
      break;
    case OMODE_24:
      outputFormat = SF_FORMAT_WAV|SF_FORMAT_PCM_24;
      break;
    case OMODE_32:
      outputFormat = SF_FORMAT_WAV|SF_FORMAT_PCM_32;
      break;
    default:
      outputFormat = SF_FORMAT_WAV|SF_FORMAT_FLOAT;
      break;
    }

  ditherMode = args.getLong("-gd");
  std::fprintf(stderr, "> ditherMode = %ld\n", ditherMode);
  GDitherType gt = (GDitherType)ditherMode;
  switch(outputMode)
    {
    case OMODE_16:
      pdither = gdither_new(gt, 2, GDither16bit, 16);
      break;
    case OMODE_24:
      pdither = gdither_new(gt, 2, GDither32bit, 24);
      break;
    case OMODE_32:
      pdither = gdither_new(gt, 2, GDither32bit, 32);
      break;
    default:
      std::fprintf(stderr, "Dithering is not used in float output mode.\n");
      pdither = NULL;
      break;
    }

  long toRate = args.getLong("-r");
  if(toRate <= 0)
    {
      std::fprintf(stderr, "!!! -r %ld \n", toRate);
      toRate = DEFAULT_TARGET_FS;
    }
  std::fprintf(stderr, "> Target Sampling Rate = %ld\n", toRate);

  long mode = args.getLong("-m");
  if(mode == 0)
    {
      std::fprintf(stderr, "!!! -m %ld \n", mode);
      mode = DEFAULT_MODE;
    }
  std::fprintf(stderr, "> Mode = %ld\n", mode);

  double kparam = args.getDouble("-k");
  if(kparam <= 0)
    {
      std::fprintf(stderr, "!!! -k %f \n", kparam);
      kparam = DEFAULT_KAISER_PARAMETER;
    }
  std::fprintf(stderr, "> Kaiser Parameter = %f\n", kparam);

  double cparam = args.getDouble("-a");
  if(cparam <= 0||cparam > 1)
    {
      std::fprintf(stderr, "!!! -a %f \n", cparam);
      cparam = DEFAULT_COSRO_PARAMETER;
    }
  std::fprintf(stderr, "> CosRO Parameter = %f\n", cparam);

  double transitionBand = args.getDouble("-t");
  if(transitionBand <= 0)
    transitionBand = DEFAULT_TRANSITION_BAND;
  std::fprintf(stderr, "> Transition Band = %f\n", transitionBand);

  long overWrite = args.getLong("-ow");
  std::fprintf(stderr, "> overWrite = %ld\n", overWrite);
  attenuation = args.getDouble("-att");
  if(attenuation == 0)
    attenuation = 1;
  std::fprintf(stderr, "> overall attenuation = %f\n", attenuation);

  long filterLength = args.getLong("-n");
  std::fprintf(stderr, "> Override Filter Length = %ld\n", filterLength);
  
  if(strlen(args.getFileArg(0)) == 0)
    {
      std::fprintf(stderr, "Input file is not specified.\n");
      exit(-1);
    }
  std::fprintf(stderr, "> Input = %s\n", args.getFileArg(0));

  if(strlen(args.getFileArg(1)) == 0)
    {
      std::fprintf(stderr, "Output file is not specified.\n");
      exit(-1);
    }
  std::fprintf(stderr, "> Output = %s\n", args.getFileArg(1));
  
  SndfileHandle sndHandle(args.getFileArg(0));
  if(sndHandle.frames() == 0||sndHandle.channels() != 2)
    {
      std::fprintf(stderr, "PCM file/channels is not valid: %s(%d).\n",
	      args.getFileArg(0), sndHandle.channels());
      exit(-1);
    }
  
  if(strcmp(args.getFileArg(0), args.getFileArg(1)) == 0)
    {
      std::fprintf(stderr, "!!! Input = Output\n");
      exit(-1);
    }

    if(overWrite == 0&&checkFileExist(args.getFileArg(1)) != 0)
    {
      std::fprintf(stderr, "!!! Cannot overwrite file: %s\n", args.getFileArg(1));
      exit(-1);
    }
  
  output = new SndfileHandle(args.getFileArg(1), SFM_WRITE,
			     outputFormat, sndHandle.channels(), toRate);
  if (output->refCount() != 1)
    {
      std::fprintf(stderr, "SndfileHandle Error: Reference count (%d) != 1.\n",
	      output->refCount());
      std::fprintf(stderr, "I/O error or permission error.\n");
      exit (-1) ;
    }
  output->setString(SF_STR_TITLE, args.getFileArg(1));

  std::fprintf(stderr, "\n");
  long gcmRate = gcmi(toRate, sndHandle.samplerate());
  long upRate = lcmi(toRate, sndHandle.samplerate());
  std::fprintf(stderr, "%d -[0]-> %ld -[LPF]-> %ld\n",
	  sndHandle.samplerate(), upRate, toRate);
  
  long fBlock = sndHandle.samplerate()/gcmRate;
  long toBlock = toRate/gcmRate;
  long upBlock = fBlock*toBlock;
  std::fprintf(stderr, "Block: [%ld]->[%ld]->[%ld]\n", fBlock, upBlock, toBlock);

  long upFactor = upBlock/fBlock;
  long downFactor = upBlock/toBlock;

  pfloat_t fc = 0.5;
  if(upFactor < downFactor)
    {
      std::fprintf(stderr, " x%ld < [x%ld]", upFactor, downFactor);
      fc = 1/(pfloat_t)downFactor/2;
    }
  else
    {
      std::fprintf(stderr, " [x%ld] > x%ld", upFactor, downFactor);
      fc = 1/(pfloat_t)upFactor/2;
    }
  std::fprintf(stderr, " ==>> LPF %f\n", fc);
  long filter = (long)(2*(double)1/fc);
  std::fprintf(stderr, " filterLength > %ld\n", filter);
  double upRatef = (double)upRate;
  double upFactorf = (double)upFactor;
  double transitionF = upFactorf*transitionBand/upRatef;
  std::fprintf(stderr, "Transition band F: %.8f\n", transitionF);
  long N = predictN(mode, transitionF, kparam, cparam);
  std::fprintf(stderr, "Estimated N: %ld\n", N);
  if(N > filter) filter = N;
  if(filterLength > 0)
    {
      if((long)(2*(double)1/fc) > filterLength)
	{
	  std::fprintf(stderr, "!!! filterLength is too short.\n");
	}
      else
	{
	  filter = filterLength;
	  std::fprintf(stderr, "FilterLength was overriden.\n");
	}
    }
  if(filter % 2 == 0) filter ++;
  
  setLPF(&reverbm, filter, fc, mode, kparam, cparam);
  
  long upGroupDelay = (filter-1)/2;
  std::fprintf(stderr, "U Group Delay = %ld\n", upGroupDelay);
  std::fprintf(stderr, "IR Latency = %ld\n", reverbm.getLatency());
  groupDelay =
    (long)((double)(upGroupDelay + reverbm.getLatency())/(double)downFactor);
  std::fprintf(stderr, "Total Group Delay + Latency = %ld\n", groupDelay);
  
  reverbm.setwet(0);
  reverbm.setwidth(1);
  processAll(&reverbm, &sndHandle, fBlock, fBlock*toBlock, toBlock);

  if(clipL != 0.0||clipR != 0.0)
    {
      std::fprintf(stderr, "Clip L max: %f\n", clipL);
      std::fprintf(stderr, "\tshould be x %f\n", fabs(1.0/clipL));
      std::fprintf(stderr, "Clip L max: %f\n", clipR);
      std::fprintf(stderr, "\tshould be x %f\n", fabs(1.0/clipR));
      std::fprintf(stderr, "Use -att to reduce clipping.\n");
    }

  delete output;
  if(pdither != NULL)
    {
      gdither_free(pdither);
    }

  std::fprintf(stderr, "\n");
  return 0;
}
