/**
 *  Simple Tank Reverb XMMS plugin
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
#include <freeverb/strev.hpp>
#include <freeverb/slot.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Simple Tank Reverb after Griesinger\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Simple Tank Reverb]";
static const char * configSectionString = "freeverb3_plugin_strev";

#ifdef PLUGDOUBLE
static fv3::strev_ DSP;
typedef fv3::slot_ SLOTP;
typedef fv3::utils_ UTILS;
#else
static fv3::strev_f DSP;
typedef fv3::slot_f SLOTP;
typedef fv3::utils_f UTILS;
#endif

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static void _srcf(long t){DSP.setOSFactor(t,FV3_SRC_LPF_IIR_2);
  fprintf(stderr, "strev.cpp: _srcf: SRCFactor: %ld, %ld\n", t, DSP.getOSFactor());};
static void _rsf(pfloat_t t){DSP.setRSFactor(t);};
static void _width(pfloat_t t){DSP.setwidth(t);};
static void _dry(pfloat_t t){DSP.setdry(t);};
static void _wet(pfloat_t t){DSP.setwet(t);};
static void _id(pfloat_t t){DSP.setPreDelay(t);};
static void _rsfac(pfloat_t t){DSP.setRSFactor(t);};
static void _decay(pfloat_t t){DSP.setrt60(t);};
static void _idiff1(pfloat_t t){DSP.setidiffusion1(t);};
static void _idiff2(pfloat_t t){DSP.setidiffusion2(t);};
static void _diff1(pfloat_t t){DSP.setdiffusion1(t);};
static void _diff2(pfloat_t t){DSP.setdiffusion2(t);};
static void _idamp(pfloat_t t){DSP.setinputdamp(t);};
static void _damp(pfloat_t t){DSP.setdamp(t);};
static void _odamp(pfloat_t t){DSP.setoutputdamp(t);};
static void _spin(pfloat_t t){DSP.setspin(t);};
static void _spind(pfloat_t t){DSP.setspindiff(t);};
static void _spinl(pfloat_t t){DSP.setspinlimit(t);};
static void _wander(pfloat_t t){DSP.setwander(t);};
static void _dccut(pfloat_t t){DSP.setdccutfreq(t);};
static void _modnoise1(pfloat_t t){DSP.setmodulationnoise1(t);};
static void _modnoise2(pfloat_t t){DSP.setmodulationnoise2(t);};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"SRC Factor","SRCFactor","X", "",      1,6,     0,   0,   1,true,false,true,kLong, (void*)_srcf,},
  {"RS Factor","RSFactor","X", "",      0.5,4,     2,   1,   0,true,false,true,kFloat, (void*)_rsf,},
  {"Dry Level","Dry","[dB]", "",       -60,10,     2,  -5,   0,true,true,false,kFloat,(void*)_dry,},
  {"Wet Level","Wet","[dB]", "",       -60,10,     2, -10,   0,true,true,false,kFloat,(void*)_wet,},
  {"Initial Delay","IDelay","[ms]","",-200,200,    0,   5,   0,true,false,true,kFloat,(void*)_id,},
  {"Stereo Width","Width","", "",       -1,1,    2,   1,   0,true,true,false,kFloat,(void*)_width,},
  {"Size Factor","SizeFactor","", "", 0.2,6,     2,   1,   0,true,false,true,kFloat, (void*)_rsfac,},
  {"RT60 (ReverbTime)","RT60","[s]", "",0,20,    2, 2.0,   0,true,true,false,kFloat,(void*)_decay,},
  {"Input Diffusion 1","IDiff1","", "", 0,1,     2, 0.75,  0,true,true,false,kFloat,(void*)_idiff1,},
  {"Input Diffusion 2","IDiff2","", "", 0,1,     2, 0.625, 0,true,true,false,kFloat,(void*)_idiff2,},
  {"Loop Diffusion 1","Diff1","", "",   0,1,     2, 0.7,   0,true,true,false,kFloat,(void*)_diff1,},
  {"Loop Diffusion 2","Diff2","", "",   0,1,     2, 0.5,   0,true,true,false,kFloat,(void*)_diff2,},
  {"Input LPF","IDamp","[Hz]", "",        0,20000, 2, 18000, 0,true,true,false,kFloat,(void*)_idamp,},
  {"Loop Damping (LPF)","Damp","[Hz]", "",0,10000, 2,  4000, 0,true,true,false,kFloat,(void*)_damp,},
  {"Output LPF","ODamp","[Hz]", "",       0,20000, 2,  5500, 0,true,true,false,kFloat,(void*)_odamp,},
  {"Spin","Spin","[Hz]", "",              0,10,    2,  0.72, 0,true,true,false,kFloat,(void*)_spin,},
  {"Spin Diff","SpinDiff","[Hz]", "",     0,1,     2,  0.11, 0,true,true,false,kFloat,(void*)_spind,},
  {"Modulation Noise (AP delayline)","MNoise1","","", 0,1,   2,  0.05, 0,true,true,false,kFloat,(void*)_modnoise1,},
  {"Modulation Noise (AP feedback)","MNoise2","","",  0,1,   2,  0.03, 0,true,true,false,kFloat,(void*)_modnoise2,},
  {"Modulation LPF Freq","SpinLimit","[Hz]","",       0,20,  2, 10,    0,true,true,false,kFloat,(void*)_spinl,},
  {"Modulation Strength","Wander","", "",             0,0.9, 2,  0.4,  0,true,true,false,kFloat,(void*)_wander,},
  {"DC Cut filter","DCCut","[Hz]", "",                0,100, 2,  4,    0,true,true,false,kFloat,(void*)_dccut,},
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
      fprintf(stderr, "strev.cpp: mod_samples: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      DSP.setOSFactor(DSP.getOSFactor());
      DSP.setSampleRate(srate);
      fprintf(stderr, "strev.cpp: mod_samples: SRCFactor: %ld\n", DSP.getOSFactor());
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  DSP.processreplace(iL,iR,oL,oR,length);
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
