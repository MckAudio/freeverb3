/**
 *  Progenitor Reverb XMMS plugin
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
#include <freeverb/progenitor2.hpp>
#include <freeverb/slot.hpp>
#include <freeverb/utils.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Progenitor Reverb after Griesinger V2\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "This plugin should be used with a early reflection plugin.\n"
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Progenitor Reverb V2]";
static const char * configSectionString = "freeverb3_plugin_progenitor";

#ifdef PLUGDOUBLE
static fv3::progenitor2_ DSP;
typedef fv3::slot_ SLOTP;
typedef fv3::utils_ UTILS;
#else
static fv3::progenitor2_f DSP;
typedef fv3::slot_f SLOTP;
typedef fv3::utils_f UTILS;
#endif

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static void _srcf(long t){DSP.setOSFactor(t,FV3_SRC_LPF_IIR_2);
  fprintf(stderr, "progenitor.cpp: _srcf: SRCFactor: %ld, %ld\n", t, DSP.getOSFactor());};
static void _width(pfloat_t t){DSP.setwidth(t);};
static void _dry(pfloat_t t){DSP.setdry(t);};
static void _wet(pfloat_t t){DSP.setwet(t);};
static void _id(pfloat_t t){DSP.setPreDelay(t);};
static void _rt60(pfloat_t t){DSP.setrt60(t);};
static void _rsfac(pfloat_t t){DSP.setRSFactor(t);};
static void _idiff1(pfloat_t t){DSP.setidiffusion1(t);};
static void _idiff2(pfloat_t t){DSP.setodiffusion1(t);};
static void _modnoise1(pfloat_t t){DSP.setmodulationnoise1(t);};
static void _modnoise2(pfloat_t t){DSP.setmodulationnoise2(t);};
static void _diff1(pfloat_t t){DSP.setdiffusion1(t);};
static void _diff2(pfloat_t t){DSP.setdiffusion2(t);};
static void _diff3(pfloat_t t){DSP.setdiffusion3(t);};
static void _diff4(pfloat_t t){DSP.setdiffusion4(t);};
static void _idamp(pfloat_t t){DSP.setinputdamp(t);};
static void _damp(pfloat_t t){DSP.setdamp(t);};
static void _damp2(pfloat_t t){DSP.setdamp2(t);};
static void _odamp(pfloat_t t){DSP.setoutputdamp(t);};
static void _odampbw(pfloat_t t){DSP.setoutputdampbw(t);};
static void _crossf(pfloat_t t){DSP.setcrossfeed(t);};
static void _spin(pfloat_t t){DSP.setspin(t);};
static void _spinl(pfloat_t t){DSP.setspinlimit(t);};
static void _wander(pfloat_t t){DSP.setwander(t);};
static void _dccut(pfloat_t t){DSP.setdccutfreq(t);};
static void _decay0(pfloat_t t){DSP.setdecay0(t);};
static void _decay1(pfloat_t t){DSP.setdecay1(t);};
static void _decay2(pfloat_t t){DSP.setdecay2(t);};
static void _decay3(pfloat_t t){DSP.setdecay3(t);};
static void _decayf(pfloat_t t){DSP.setdecayf(t);};
static void _bassb(pfloat_t t){DSP.setbassboost(t);};
static void _bassbw(pfloat_t t){DSP.setbassbw(t);};
static void _spin2(pfloat_t t){DSP.setspin2(t);};
static void _spinl2(pfloat_t t){DSP.setspinlimit2(t);};
static void _wander2(pfloat_t t){DSP.setwander2(t);};
static void _spin2wander(pfloat_t t){DSP.setspin2wander(t);};
static void _revtype(bool t){if(t){DSP.setReverbType(FV3_REVTYPE_PROG);}else{DSP.setReverbType(FV3_REVTYPE_SELF);}};
static void _rsf(pfloat_t t){DSP.setRSFactor(t);};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"SRC Factor","SRCFactor","", "",    1,6,     0,   0,   2,true,false,true,kLong, (void*)_srcf,},
  {"RS Factor","RSFactor","X", "",   0.5,4,     2,   1,   0,true,false,true,kFloat, (void*)_rsf,},
  {"Dry","Dry","[dB]", "",           -60,10,    2,  -3,   0,true,true,false,kFloat,(void*)_dry,},
  {"Wet","Wet","[dB]", "",           -60,10,    2,   0,   0,true,true,false,kFloat,(void*)_wet,},
  {"Initial Delay","IDelay","[ms]","",-200,200, 1,   1,   0,true,false,true,kFloat,(void*)_id,},
  {"Width","Width","", "",             0,1,     2,   1,   0,true,true,false,kFloat,(void*)_width,},
  {"Size Factor","SizeFactor","", "",0.2,6,     2,   1,   0,true,false,true,kFloat, (void*)_rsfac,},
  {"Reverb Time RT60","RT60","[s]", "",0.2,15,  2, 2.8,   0,true,true,false,kFloat,(void*)_rt60,},
  {"Decay 0 (loop)","Decay0","", "",   0,2,     3, 0.200, 0,true,true,false,kFloat,(void*)_decay0,},
  {"Decay 1","Decay1","", "",          0,1,     3, 0.938, 0,true,true,false,kFloat,(void*)_decay1,},
  {"Decay 2","Decay2","", "",          0,1,     3, 0.844, 0,true,true,false,kFloat,(void*)_decay2,},
  {"Decay 3","Decay3","", "",          0,1,     3, 0.906, 0,true,true,false,kFloat,(void*)_decay3,},
  {"Decay F","DecayF","", "",          0,4,     3, 1.000, 0,true,true,false,kFloat,(void*)_decayf,},
  {"Diffusion 1","Diff1","", "",       0,1,     3, 0.375, 0,true,true,false,kFloat,(void*)_diff1,},
  {"Diffusion 2","Diff2","", "",       0,1,     3, 0.312, 0,true,true,false,kFloat,(void*)_diff2,},
  {"Diffusion 3","Diff3","", "",       0,1,     3, 0.406, 0,true,true,false,kFloat,(void*)_diff3,},
  {"Diffusion 4","Diff4","", "",       0,1,     3, 0.25,  0,true,true,false,kFloat,(void*)_diff4,},
  {"Input Diffusion 1","IDiff1","","",-1,1,     3, 0.78,  0,true,true,false,kFloat,(void*)_idiff1,},
  {"Input Diffusion 2","IDiff2","","",-1,1,     3, 0.78,  0,true,true,false,kFloat,(void*)_idiff2,},
  {"Input Crossfeed","ICrossF","","",  0,1,     3, 0.4,   0,true,true,false,kFloat,(void*)_crossf,},
  {"Input DC Cut Freq","DCCut","[Hz]", "", 0,100,   2,     3, 0,true,true,false,kFloat,(void*)_dccut,},
  {"Input LPF","IDamp","[Hz]", "",        10,24000, 2, 18000, 0,true,true,false,kFloat,(void*)_idamp,},
  {"Loop LPF","Damp","[Hz]", "",          10,24000, 2,  8000, 0,true,true,false,kFloat,(void*)_damp,},
  {"Output LPF","ODamp","[Hz]", "",       10,24000, 2, 16000, 0,true,true,false,kFloat,(void*)_odamp,},
  {"Output LPF Bandwidth","ODampBW","[Oct]","",    1,8, 2, 2,     0,true,true,false,kFloat,(void*)_odampbw,},
  {"Bass Boost LPF Bandwidth","Damp2BW","[Oct]","",1,8, 2, 2,     0,true,true,false,kFloat,(void*)_bassbw,},
  {"Bass Boost LPF","Damp2","[Hz]","",         10,1000, 2, 300,   0,true,true,false,kFloat,(void*)_damp2,},
  {"Bass Boost","BassBoost","", "",    0,1,     2, 0.45,   0,true,true,false,kFloat,(void*)_bassb,},
  {"Modulation Frequency","Spin","[Hz]", "",           0,5,  2, 0.7, 0,true,true,false,kFloat,(void*)_spin,},
  {"Modulation Noise (AP delayline)","MNoise1","","",  0,0.5,2, 0.09,0,true,true,false,kFloat,(void*)_modnoise1,},
  {"Modulation Noise (AP feedback)","MNoise2","","",   0,0.5,2, 0.06,0,true,true,false,kFloat,(void*)_modnoise2,},
  {"Modulation Frequency Limit","SpinLimit","[Hz]","", 0,20, 2,10,   0,true,true,false,kFloat,(void*)_spinl,},
  {"Modulation Strength","Wander","", "",              0,0.9,2, 0.25,0,true,true,false,kFloat,(void*)_wander,},
  {"Spin","Spin2","[Hz]", "",                0,5,  2, 2.4, 0,true,true,false,kFloat,(void*)_spin2,},
  {"Spin Limit","SpinLimit2","[Hz]","",      0,20, 2, 5,   0,true,true,false,kFloat,(void*)_spinl2,},
  {"Spin Strength","Wander2","", "",                   0,0.9,2, 0.3, 0,true,true,false,kFloat,(void*)_wander2,},
  {"Wander","Spin2Freq","[ms]", "",                    0,100,2, 22,  0,true,false,true,kFloat,(void*)_spin2wander,},
  {"Reverb Type", "RevType","", "",    0,0,     0,   0,   0,false,true,false,kBool,(void*)_revtype},
};

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
      fprintf(stderr, "progenitor.cpp: mod_samples: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      DSP.setOSFactor(DSP.getOSFactor());
      DSP.setSampleRate(srate);
      fprintf(stderr, "progenitor.cpp: mod_samples: SRCFactor: %ld\n", DSP.getOSFactor());
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  DSP.processreplace(iL,iR,oL,oR,length);
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
