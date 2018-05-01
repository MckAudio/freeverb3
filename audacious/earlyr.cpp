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
#include <freeverb/delay.hpp>
#include <freeverb/efilter.hpp>
#include <freeverb/biquad.hpp>
#include <freeverb/slot.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

static const char *about_text =
  "Freeverb3 "VERSION"\n"
  "Early Reflection V1\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "This plugin should be used with other reverb plugins.\n"
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";
static const char * productString = "Freeverb3 "VERSION" [Early Reflection V1]";
static const char * configSectionString = "freeverb3_plugin_earlyr";

#ifdef PLUGDOUBLE
static fv3::delay_ DSPL, DSPR, DSPLR, DSPRL;
static fv3::biquad_ ALLPASSL, ALLPASSR, ALLPASSL2, ALLPASSR2;
typedef fv3::utils_ UTILS;
#else
static fv3::delay_f DSPL, DSPR, DSPLR, DSPRL;
static fv3::biquad_f ALLPASSL, ALLPASSR, ALLPASSL2, ALLPASSR2;
typedef fv3::utils_f UTILS;
#endif

static int currentfs = 0;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
static fv3::libxmmsplugin *XMMSPlugin = NULL;

static pfloat_t _dry_r = 1, _wet_r = 1, _cross_r = 1;
static void _dry(pfloat_t t){_dry_r = t;};
static void _wet(pfloat_t t){_wet_r = t;};
static void _cro(pfloat_t t){_cross_r = t;};

// configurations
static PluginParameterTable ppConfTable[] = {
  {"Dry","Dry","", "",        0,2, 2, 0.5, 0,true,true,false,kFloat,(void*)_dry,},
  {"Wet","Wet","", "",        0,2, 2, 0.5, 0,true,true,false,kFloat,(void*)_wet,},
  {"Cross","Cross","", "",    0,2, 2, 0.2, 0,true,true,false,kFloat,(void*)_cro,},
};

#define LRDIFF 0.001   // 1ms
#define TAPMAX 6      // 18tap
#define LRDELAY 0.0002 // 0.1~0.3ms
static pfloat_t coefficientsBaseTable[TAPMAX] =
  { 1.020, 0.818, 0.635, 0.719, 0.267, 0.242, };
static pfloat_t coefficientsDiffTable[TAPMAX] =
  { 0.001, 0.002,-0.002, 0.003,-0.080, 0.001, };
static pfloat_t coefficientsTableL[TAPMAX], coefficientsTableR[TAPMAX];

static pfloat_t timeCoefficientsTable[TAPMAX] =
  { .0199, .0354, .0389, .0414, .0699, 0.0796, };
static long timeCoefficientsTableL[TAPMAX], timeCoefficientsTableR[TAPMAX];

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
      long maxLength = (long)(timeCoefficientsTable[TAPMAX-1]*(pfloat_t)currentfs)*2;
      long lrDelay = (long)((pfloat_t)LRDELAY*(pfloat_t)currentfs);
      DSPL.setsize(maxLength); DSPR.setsize(maxLength);
      DSPLR.setsize(lrDelay);  DSPRL.setsize(lrDelay);
      ALLPASSL.setAPF_RBJ(700., 4., currentfs, FV3_BIQUAD_RBJ_BW);
      ALLPASSL.printconfig();
      ALLPASSR.setAPF_RBJ(700., 4., currentfs, FV3_BIQUAD_RBJ_BW);
      ALLPASSR.printconfig();
      ALLPASSL2.setAPF_RBJ(800., 4., currentfs, FV3_BIQUAD_RBJ_BW);
      ALLPASSL2.printconfig();
      ALLPASSR2.setAPF_RBJ(800., 4., currentfs, FV3_BIQUAD_RBJ_BW);
      ALLPASSR2.printconfig();
      ALLPASSL.mute(); ALLPASSR.mute(); ALLPASSL2.mute(); ALLPASSR2.mute();
      DSPL.mute(), DSPR.mute(), DSPLR.mute(), DSPRL.mute();
      // update index table
      for(long j = 0;j < TAPMAX;j ++)
	{
	  coefficientsTableL[j] = coefficientsBaseTable[j];
	  //coefficientsTableR[j] = coefficientsBaseTable[j];
	  coefficientsTableR[j] = coefficientsBaseTable[j] + coefficientsDiffTable[j];
	  timeCoefficientsTableL[j] = (long)(timeCoefficientsTable[j]*(pfloat_t)currentfs);
	  //timeCoefficientsTableR[j] = (long)(timeCoefficientsTable[j]*(pfloat_t)currentfs);
	  timeCoefficientsTableR[j] = (long)((timeCoefficientsTable[j]+LRDIFF)*(pfloat_t)currentfs);
	}
    }
  XMMSPlugin->callNRTParameters();
  uint32_t mxcsr = UTILS::getMXCSR();
  UTILS::setMXCSR(FV3_X86SIMD_MXCSR_FZ|FV3_X86SIMD_MXCSR_DAZ|FV3_X86SIMD_MXCSR_EMASK_ALL);
  for(long i = 0;i < length;i ++)
    {
      oL[i] = _dry_r * iL[i]; oR[i] = _dry_r * iR[i];
      DSPL.process(iL[i]); DSPR.process(iR[i]);
      pfloat_t wetL = 0, wetR = 0;
      for(long j = 0;j < TAPMAX;j ++)
	{
	  wetL += coefficientsTableL[j]*DSPL.get_z(timeCoefficientsTableL[j]);
	  wetR += coefficientsTableR[j]*DSPR.get_z(timeCoefficientsTableR[j]);
	}
      oL[i] += ALLPASSL2(wetL * _wet_r + ALLPASSL(_cross_r * DSPRL.process(iR[i] + wetR)));
      oR[i] += ALLPASSR2(wetR * _wet_r + ALLPASSR(_cross_r * DSPLR.process(iL[i] + wetL)));
    }
  UTILS::setMXCSR(mxcsr);
  pthread_mutex_unlock(&plugin_mutex);
}

#include "libxmmsplugin_table.hpp"
