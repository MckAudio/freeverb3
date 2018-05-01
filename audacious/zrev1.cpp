/**
 *  ZRev Test XMMS plugin
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
#include <freeverb/zrev.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "ZRev1\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Rev1]";
static const char * configSectionString = "freeverb3_plugin_zrev1";

#ifdef PLUGDOUBLE
static fv3::zrev_ DSP;
typedef fv3::utils_ UTILS;
#else
static fv3::zrev_f DSP;
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

static void _ldamp(pfloat_t t){DSP.setloopdamp(t);};
static void _odamp(pfloat_t t){DSP.setoutputlpf(t);};

static void _apfee(pfloat_t t){DSP.setapfeedback(t);};
static void _rt60(pfloat_t t){DSP.setrt60(t);};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"SRC Factor","SRCFactor","X", "",      1,6,     0,   0,   1,true,false,true,kLong, (void*)_srcf,},
  {"RS Factor","RSFactor","X", "",      0.5,4,     2,   1,   0,true,false,true,kFloat, (void*)_rsf,},
  {"Dry Level","Dry","[dB]", "",       -60,10,     2,  -5,   0,true,true,false,kFloat,(void*)_dry,},
  {"Wet Level","Wet","[dB]", "",       -60,10,     2,  -5,   0,true,true,false,kFloat,(void*)_wet,},
  {"Stereo Width","Width","", "",       -1,1,      2,   1,   0,true,true,false,kFloat,(void*)_width,},
  {"Loop Damping (LPF)","Damp","[Hz]", "",0,10000, 2,  6000, 0,true,true,false,kFloat,(void*)_ldamp,},
  {"Output LPF","ODamp","[Hz]", "",       0,20000, 2, 16000, 0,true,true,false,kFloat,(void*)_odamp,},
  {"AP Feedback","APFeedback","", "",  0,1,     2, 0.6,   0,true,true,false,kFloat,(void*)_apfee,},
  {"Reverb Time RT60","RT60","[s]", "",0.2,15,  2, 2.5,   0,true,true,false,kFloat,(void*)_rt60,},
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
      fprintf(stderr, "zrev1.cpp: mod_samples: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      DSP.setOSFactor(DSP.getOSFactor());
      DSP.setSampleRate(srate);
      fprintf(stderr, "zrev1.cpp: mod_samples: SRCFactor: %ld\n", DSP.getOSFactor());
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  DSP.processreplace(iL,iR,oL,oR,length);
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
