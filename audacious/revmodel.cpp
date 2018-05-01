/**
 *  Freeverb XMMS plugin
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
#include <freeverb/revmodel.hpp>
#include <freeverb/utils.hpp>

#define DEFAULTMAXVFS 192000
#define DEFAULTFS 44100

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Freeverb\n"
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
  "http://www.nongnu.org/freeverb3/\n"
  "Original Freeverb\n"
  "Copyright (C) 2000 Jezar at Dreampoint\n";
static const char * productString = "Freeverb3 "VERSION" [Freeverb]";
static const char * configSectionString = "freeverb3_plugin_revmodel";

#ifdef PLUGDOUBLE
static fv3::revmodel_ DSP;
typedef fv3::slot_ SLOTP;
typedef fv3::utils_ UTILS;
#else
static fv3::revmodel_f DSP;
typedef fv3::slot_f SLOTP;
typedef fv3::utils_f UTILS;
#endif

static long converter_type = FV3_SRC_LPF_IIR_2;

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static void _wet(pfloat_t t){DSP.setwet(t);};
static void _dry(pfloat_t t){DSP.setdry(t);};
static void _roomsize(pfloat_t t){DSP.setroomsize(t);};
static void _damp(pfloat_t t){DSP.setdamp(t);};
static void _width(pfloat_t t){DSP.setwidth(t);};
static void _idelay(pfloat_t t){DSP.setPreDelay(t);};
static void _factor(long t){DSP.setOSFactor(t,converter_type);};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"Dry","dry","", "",                 0.0,1.0,  2,  0.4, 0,true,true,false,kFloat,(void*)_dry,},
  {"Wet","wet","", "",                 0.0,1.0,  2,  0.3, 0,true,true,false,kFloat,(void*)_wet,},
  {"Damp","damp","", "",               0.0,1.0,  2,  0.2, 0,true,true,false,kFloat,(void*)_damp,},
  {"Roomsize","roomsize","", "",       0.0,1.0,  2,  0.7, 0,true,true,false,kFloat,(void*)_roomsize,},
  {"Width","width","", "",             0.0,1.0,  2,  0.9, 0,true,true,false,kFloat,(void*)_width,},
  {"Initial Delay","idelay","", "", -200.0,200.0,0, 10.0, 0,true,false,true,kFloat,(void*)_idelay,},
  {"OSFactor","osfactor","", "",         1,6,    0,   0,  2,true,false,true,kLong, (void*)_factor,},
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
      fprintf(stderr, "reverbm.cpp: Fs %d -> %d, resetAll\n", currentfs, srate);
      currentfs = srate;
      DSP.setSampleRate(srate);
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  DSP.processreplace(iL,iR,oL,oR,length);
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
