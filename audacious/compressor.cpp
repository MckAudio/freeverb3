/**
 *  Compressor XMMS plugin
 *
 *  Copyright (C) 2006-2018 Teru Kamogashira
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
#include "libxmmsplugin.hpp"
#include <freeverb/fir3bandsplit.hpp>
#include <freeverb/compmodel.hpp>
#include <freeverb/limitmodel.hpp>
#include <freeverb/utils.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Linear Phase MultiBand Compressor\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Zf MultiBand Compressor]";
static const char * configSectionString = "freeverb3_plugin_zmbcompressor";

#ifdef PLUGDOUBLE
static fv3::fir3bandsplit_ split;
static fv3::compmodel_ compL, compM, compH;
static fv3::limitmodel_ limit;
typedef fv3::utils_ UTILS;
typedef fv3::slot_ SLOTP;
#else
static fv3::fir3bandsplit_f split;
static fv3::compmodel_f compL, compM, compH;
static fv3::limitmodel_f limit;
typedef fv3::utils_f UTILS;
typedef fv3::slot_f SLOTP;
#endif

static pfloat_t gainL=1, gainM=1, gainH=1;
static pfloat_t againL=1, againM=1, againH=1;

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static float dB2real(float dB){return pow(10.0, dB/20.0);}
static pfloat_t aGC(pfloat_t thr, pfloat_t ratio){return (pfloat_t)(1.0/dB2real((1-1/ratio)*thr));}
static void lfdiv(pfloat_t t){split.setLowFreqDivider(t);}
static void hfdiv(pfloat_t t){split.setHighFreqDivider(t);}
static void fband(pfloat_t t){split.setTransitionBand(t);
  fprintf(stderr, "compressor.cpp: Latency=%ld GroupDelay=%ld Filter=%ld\n",
	  split.getLatency(),split.getGroupDelay(),split.getFilterLength());}
static void fwind(long t){split.setWindow(t+1); fprintf(stderr, "compressor.cpp: Filter=%ld\n",split.getFilterLength());}
static void fpara(pfloat_t t){split.setParameter(t);}
// 3band general
static void ccrms(pfloat_t t)
{compL.setRMS(t);compM.setRMS(t);compH.setRMS(t);compL.mute(); compM.mute(); compH.mute();}
static void ccloo(pfloat_t t)
{compL.setLookahead(t);compM.setLookahead(t);compH.setLookahead(t);compL.mute(); compM.mute(); compH.mute();}
// low band
static void lcatt(pfloat_t t){compL.setAttack(t);}
static void lcrel(pfloat_t t){compL.setRelease(t);}
static void lcthr(pfloat_t t){compL.setThreshold(t);againL = aGC(t,compL.getRatio());}
static void lcsof(pfloat_t t){compL.setSoftKnee(t);}
static void lcrat(pfloat_t t){compL.setRatio(t);againL = aGC(compL.getThreshold(),t);}
static void lcgai(pfloat_t t){gainL = dB2real(t);}
// mid band
static void mcatt(pfloat_t t){compM.setAttack(t);}
static void mcrel(pfloat_t t){compM.setRelease(t);}
static void mcthr(pfloat_t t){compM.setThreshold(t);againM = aGC(t,compM.getRatio());}
static void mcsof(pfloat_t t){compM.setSoftKnee(t);}
static void mcrat(pfloat_t t){compM.setRatio(t);againM = aGC(compM.getThreshold(),t);}
static void mcgai(pfloat_t t){gainM = dB2real(t);}
// high band
static void hcatt(pfloat_t t){compH.setAttack(t);}
static void hcrel(pfloat_t t){compH.setRelease(t);}
static void hcthr(pfloat_t t){compH.setThreshold(t);againH = aGC(t,compH.getRatio());}
static void hcsof(pfloat_t t){compH.setSoftKnee(t);}
static void hcrat(pfloat_t t){compH.setRatio(t);againH = aGC(compH.getThreshold(),t);}
static void hcgai(pfloat_t t){gainH = dB2real(t);}
// limiter
static void lrms(pfloat_t t){limit.setRMS(t); limit.mute();}
static void lloo(pfloat_t t){limit.setLookahead(t); limit.mute();}
static void llor(pfloat_t t){limit.setLookaheadRatio(t);}
static void latt(pfloat_t t){limit.setAttack(t);}
static void lrel(pfloat_t t){limit.setRelease(t);}
static void lthr(pfloat_t t){limit.setThreshold(t);}
static void lcei(pfloat_t t){limit.setCeiling(t);}

// configurations
static PluginParameterTable ppConfTable[] = {
  {"LowFreqDivider","lowfreq","[Hz]", "",   50.0,10000.0,1, 140, 0,true, false,false,kFloat,(void*)lfdiv,},
  {"HighFreqDivider","highfreq","[Hz]", "", 50.0,10000.0,1,6300, 0,true, false,false,kFloat,(void*)hfdiv,},
  {"TransitionBand","bandfreq","[Hz]", "", 100.0,1000.0, 1, 200, 0,true, false,false,kFloat,(void*)fband,},
  {"Filter Window","window","W", "Blackman/Hanning/Hamming/Kaiser/CosRO/Square",
   0,0,0,0,0,true,false,false,kSelect,(void*)fwind,},
  {"Filter Parameter","fparameter","", "",   0.0,100.0,  2,   0, 0,true,false,false,kFloat,(void*)fpara,},

  // compressors
  {"All Comp RMS","cc_rms","[ms]", "",         0.0,100.0,2, 30, 0,true,false,false,kFloat,(void*)ccrms,},
  {"All Comp Lookahead","cc_looka","[ms]", "", 0.0,100.0,2, 10, 0,true,false,false,kFloat,(void*)ccloo,},
  
  {"Low Comp Attack","lc_attack","[ms]", "",         0.0,10000.0,2,  0,0,true,true,false,kFloat,(void*)lcatt,},
  {"Low Comp Release","lc_release","[ms]", "",       0.0,10000.0,2, 50,0,true,true,false,kFloat,(void*)lcrel,},
  {"Low Comp Threshold","lc_threshold","[dB]", "",-100.0,0.0,    2,-27,0,true,true,false,kFloat,(void*)lcthr,},
  {"Low Comp SoftKnee","lc_softknee","[dB]", "",     0.0,20.0,   2, 10,0,true,true,false,kFloat,(void*)lcsof,},
  {"Low Comp Ratio","lc_ratio","[X]", "",            0.1,20.0,   2,  5,0,true,true,false,kFloat,(void*)lcrat,},
  {"Low Output Gain","lc_gain","[dB]", "",        -120.0,20.0,   2, -4,0,true,true,false,kFloat,(void*)lcgai,},

  {"Mid Comp Attack","mc_attack","[ms]", "",         0.0,10000.0,2,  0,0,true,true,false,kFloat,(void*)mcatt,},
  {"Mid Comp Release","mc_release","[ms]", "",       0.0,10000.0,2, 80,0,true,true,false,kFloat,(void*)mcrel,},
  {"Mid Comp Threshold","mc_threshold","[dB]", "",-100.0,0.0,    2,-17,0,true,true,false,kFloat,(void*)mcthr,},
  {"Mid Comp SoftKnee","mc_softknee","[dB]", "",     0.0,20.0,   2, 10,0,true,true,false,kFloat,(void*)mcsof,},
  {"Mid Comp Ratio","mc_ratio","[X]", "",            0.1,20.0,   2,  3,0,true,true,false,kFloat,(void*)mcrat,},
  {"Mid Output Gain","mc_gain","[dB]", "",        -120.0,20.0,   2, -4,0,true,true,false,kFloat,(void*)mcgai,},

  {"High Attack","hc_attack","[ms]", "",             0.0,10000.0,2,  0,0,true,true,false,kFloat,(void*)hcatt,},
  {"High Comp Release","hc_release","[ms]", "",      0.0,10000.0,2, 80,0,true,true,false,kFloat,(void*)hcrel,},
  {"High Comp Threshold","hc_threshold","[dB]", "",-100.0,0.0,   2,-20,0,true,true,false,kFloat,(void*)hcthr,},
  {"High Comp SoftKnee","hc_softknee","[dB]", "",    0.0,20.0,   2, 10,0,true,true,false,kFloat,(void*)hcsof,},
  {"High Comp Ratio","hc_ratio","[X]", "",           0.1,20.0,   2,  7,0,true,true,false,kFloat,(void*)hcrat,},
  {"High Output Gain","hc_gain","[dB]", "",       -120.0,20.0,   2, -4,0,true,true,false,kFloat,(void*)hcgai,},

  // limiter
  {"Limiter RMS","l_rms","[ms]", "",                 0.0,100.0, 2,   0,0,true,false,false,kFloat,(void*)lrms,},
  {"Limiter Lookahead","l_looka","[ms]", "",         0.0,15.0,  2,   1,0,true,false,false,kFloat,(void*)lloo,},
  {"Limiter LookaheadRatio","l_lookara","[X]", "",   0.1,3.0,   2,   1,0,true,true,false,kFloat,(void*)llor,},
  {"Limiter Attack","l_attack","[ms]", "",           0.0,100.0, 2,   0,0,true,true,false,kFloat,(void*)latt,},
  {"Limiter Release","l_release","[ms]", "",         0.0,1000.0,2, 100,0,true,true,false,kFloat,(void*)lrel,},
  {"Limiter Threshold","l_threshold","[dB]", "",  -100.0,0,     2,-0.5,0,true,true,false,kFloat,(void*)lthr,},
  {"Limiter Ceiling","l_ceiling","[dB]", "",      -100.0,0.0,   2,   0,0,true,true,false,kFloat,(void*)lcei,},
};

static SLOTP revSum, T1, T2, T3, O1, O2, O3;
static void mod_samples(pfloat_t * iL, pfloat_t * iR, pfloat_t * oL, pfloat_t * oR, gint length, gint srate)
{
  if(pthread_mutex_trylock(&plugin_mutex) == EBUSY) return;
  if(plugin_available != true||XMMSPlugin == NULL)
    {
      pthread_mutex_unlock(&plugin_mutex);
      return;
    }

  if(currentfs != srate)
    {
      fprintf(stderr, "compressor.cpp: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      split.setSampleRate(srate);
      compL.setSampleRate(srate);
      compM.setSampleRate(srate);
      compH.setSampleRate(srate);
      limit.setSampleRate(srate);
      // need to reload config...
    }

  gboolean needMute = XMMSPlugin->callNRTParameters();
  if(needMute == true)
    {
      split.mute();compL.mute();compM.mute();compH.mute();limit.mute();
    }

  if(revSum.getsize() < length)
    {
      revSum.alloc(length, 2);
      T1.alloc(length, 2);T2.alloc(length, 2);T3.alloc(length, 2);
      O1.alloc(length, 2);O2.alloc(length, 2);O3.alloc(length, 2);
    }

  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);

  split.splitR(iL,iR,T1.L,T1.R,T2.L,T2.R,T3.L,T3.R,length);
  compL.processreplace(T1.L,T1.R,O1.L,O1.R,length);
  compM.processreplace(T2.L,T2.R,O2.L,O2.R,length);
  compH.processreplace(T3.L,T3.R,O3.L,O3.R,length);
  split.mergeR(O1.L,O1.R,O2.L,O2.R,O3.L,O3.R,revSum.L,revSum.R,againL*gainL,againM*gainM,againH*gainH,length);
  limit.processreplace(revSum.L,revSum.R,oL,oR,length);

  UTILS::setMXCSR(mxcsr);

  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
