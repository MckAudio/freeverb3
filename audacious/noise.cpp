/**
 *  Early Reflection Test XMMS plugin
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
#include <freeverb/efilter.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Noise Generator\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Noise Generator]";
static const char * configSectionString = "freeverb3_plugin_noise";

#ifdef PLUGDOUBLE
static fv3::noisegen_pink_frac_ DSPL, DSPR;
typedef fv3::utils_ UTILS;
#else
static fv3::noisegen_pink_frac_f DSPL, DSPR;
typedef fv3::utils_f UTILS;
#endif

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static pfloat_t _dry_r = 1, _wet_r = 1;
static void _dry(pfloat_t t){_dry_r = t;};
static void _wet(pfloat_t t){_wet_r = t;};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"Dry","Dry","", "",0,1, 2, 0.0, 0,true,true,false,kFloat,(void*)_dry,},
  {"Wet","Wet","", "",0,1, 2, 1.0, 0,true,true,false,kFloat,(void*)_wet,},
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
      fprintf(stderr, "earlyr.cpp: mod_samples: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  for(long i = 0;i < length;i ++)
    {
      oL[i] = _dry_r * iL[i] + _wet_r * DSPL.process();
      oR[i] = _dry_r * iR[i] + _wet_r * DSPR.process();
    }
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
