/**
 *  NRev Test XMMS plugin
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
#include <freeverb/nrevb.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "NReverb V2\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
#ifdef PLUGINIT
  "Self Plugin Init/Cleanup\n"
#endif
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [NReverb V3]";
static const char * configSectionString = "freeverb3_plugin_nrev";

#ifdef PLUGDOUBLE
static fv3::nrevb_ DSP;
typedef fv3::utils_ UTILS;
#else
static fv3::nrevb_f DSP;
typedef fv3::utils_f UTILS;
#endif

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static void _srcf(long t){DSP.setOSFactor(t,FV3_SRC_LPF_IIR_2); fprintf(stderr, "strev.cpp: _srcf: SRCFactor: %ld, %ld\n", t, DSP.getOSFactor());};
static void _rsf(pfloat_t t){DSP.setRSFactor(t);};

static void _dry(pfloat_t t){DSP.setdry(t);};
static void _wet(pfloat_t t){DSP.setwet(t);};
static void _width(pfloat_t t){DSP.setwidth(t);};
static void _id(pfloat_t t){DSP.setPreDelay(t);};

static void _rt60(pfloat_t t){DSP.setrt60(t);};
static void _damp1(pfloat_t t){DSP.setdamp(t);};
static void _damp2(pfloat_t t){DSP.setdamp2(t);};
static void _damp3(pfloat_t t){DSP.setdamp3(t);};
static void _fb(pfloat_t t){DSP.setfeedback(t);};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"SRC Factor","SRCFactor","X", "",      1,6,     0,   0,   1,true,false,true,kLong, (void*)_srcf,},
  {"RS Factor","RSFactor","X", "",      0.5,4,     2,   1,   0,true,false,true,kFloat, (void*)_rsf,},
  {"Dry Level","Dry","[dB]", "",       -60,10,     2,  -5,   0,true,true,false,kFloat,(void*)_dry,},
  {"Wet Level","Wet","[dB]", "",       -60,10,     2,  -5,   0,true,true,false,kFloat,(void*)_wet,},
  {"Stereo Width","Width","", "",       -1,1,      2,   1,   0,true,true,false,kFloat,(void*)_width,},
  {"Initial Delay","IDelay","[ms]","",-200,200,    0,   5,   0,true,false,true,kFloat,(void*)_id,},

  {"Reverb Time RT60","RT60","[s]", "",0.2,15,  2, 2.5,   0,true,true,false,kFloat,(void*)_rt60,},
  {"Damp1","damp1","", "", 0,1,     2, 0.6,   0,true,true,false,kFloat,(void*)_damp1,},
  {"Damp2","damp2","", "", 0,1,     2, 0.6,   0,true,true,false,kFloat,(void*)_damp2,},
  {"Damp3","damp3","", "", 0,1,     2, 0.6,   0,true,true,false,kFloat,(void*)_damp3,},
  {"Feedback","feedback","", "", 0,1,     2, 0.75,  0,true,true,false,kFloat,(void*)_fb,},
};

// preset
static char presetName[][32] = {
  "Hall 1",
  "Hall 2",
  "Room 1",
  "Room 2",
  "Stadium",
  "Drum Chamber",};
static float presetI[][9] = {
  {2.05, -10.0, -2.0,  0.65, 0.3,  0.7,   0.43, 0.9, 10,},
  {3.05, -15.0, -2.0,  0.6,  0.85, 0.85,  0.10, 1.0, 15,},
  {1.0,  -10.0, -2.0,  0.7,  0.4,  0.2,   0.25, 0.9,  1,},
  {1.0,  -10.0, -2.0,  0.6,  0.9,  0.9,   0.10, 1.0,  0,},
  {2.98, -15.0, -2.0,  0.6,  0.21, 0.6,   0.05, 1.0, 20,},
  {1.07, -10.0, -2.0,  0.56,  0.9,  0.9,   0.2, 1.0,  5,},
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
      fprintf(stderr, "nrev.cpp: mod_samples: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      DSP.setSampleRate(srate);
      fprintf(stderr, "nrev.cpp: mod_samples: SRCFactor: %ld\n", DSP.getOSFactor());
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  DSP.processreplace(iL,iR,oL,oR,length);
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
