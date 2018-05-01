/**
 *  Early Reflection XMMS plugin
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
#include <freeverb/earlyref.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Early Reflection V2\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "This plugin should be used with other reverb plugins.\n"
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Early Reflection V2]";
static const char * configSectionString = "freeverb3_plugin_earlyref2";

#ifdef PLUGDOUBLE
static fv3::earlyref_ DSP;
typedef fv3::utils_ UTILS;
#else
static fv3::earlyref_f DSP;
typedef fv3::utils_f UTILS;
#endif

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static void _dry(pfloat_t t){DSP.setdry(t);};
static void _wet(pfloat_t t){DSP.setwet(t);};
static void _width(pfloat_t t){DSP.setwidth(t);};
static void _id(pfloat_t t){DSP.setPreDelay(t); DSP.mute();};
static void _lrdelay(pfloat_t t){DSP.setLRDelay(t); DSP.mute();};
static void _lrcrossap(pfloat_t t){DSP.setLRCrossApFreq(t,4);};
static void _diffap(pfloat_t t){DSP.setDiffusionApFreq(t,4);};
static void _factor(pfloat_t t){DSP.setRSFactor(t); DSP.mute();};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"Dry","Dry","[dB]", "",          -100,20,   2,  -5,    0,true,true,false,kFloat,(void*)_dry,},
  {"Wet","Wet","[dB]", "",          -100,20,   2,  -8,    0,true,true,false,kFloat,(void*)_wet,},
  {"Cross Width","CrossWidth","", "",  -1,1,   2, 0.2,    0,true,true,false,kFloat,(void*)_width,},
  {"Pre Delay","PreDelay","[ms]", "",  -4,4,   2,   0,    0,true,false,true,kFloat,(void*)_id,},
  {"LR Delay","LRDelay","[ms]", "",     0,1,   2, 0.3,    0,true,false,true,kFloat,(void*)_lrdelay,},
  {"LR Cross Ap Fq","LRCrossApFq","[Hz]", "",   10,4000,0,750,0,true,true,false,kFloat,(void*)_lrcrossap,},
  {"Diffusion Ap Fq","DiffusionApFq","[Hz]", "",10,4000,0,150,0,true,true,false,kFloat,(void*)_diffap,},
  {"Factor","Factor","[X]", "",       0.2,4,   2, 1.2,    0,true,false,true,kFloat,(void*)_factor,},
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
      fprintf(stderr, "earlyr2.cpp: mod_samples: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      DSP.setSampleRate(srate);
      fprintf(stderr, "earlyr2.cpp: mod_samples: SRCFactor: %ld\n", DSP.getOSFactor());
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  DSP.processreplace(iL,iR,oL,oR,length);
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
