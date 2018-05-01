/**
 *  XMMS/BMP/audacious plugin Framework
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

#include "so.h"

static void
#ifdef __GNUC__
__attribute__ ((constructor))
#endif
plugin_init(void)
{
  fprintf(stderr, "libxmmsplugin: plugin_init(): %s\n", configSectionString);
  pthread_mutex_init(&plugin_mutex, NULL);
}

static void
#ifdef __GNUC__
__attribute__ ((destructor))
#endif
plugin_fini(void)
{
  fprintf(stderr, "libxmmsplugin: plugin_fini(): %s\n", configSectionString);
  pthread_mutex_destroy(&plugin_mutex);
}

static gboolean init(void)
{
#ifndef __GNUC__
  plugin_init();
#endif
  fprintf(stderr, "libxmmsplugin: init(): %s\n", configSectionString);
  pthread_mutex_lock(&plugin_mutex);
  plugin_available = true;
  if(XMMSPlugin != NULL) delete XMMSPlugin;
  XMMSPlugin = new fv3::libxmmsplugin(ppConfTable, sizeof(ppConfTable)/sizeof(PluginParameterTable),
				      about_text, productString, configSectionString);  
  XMMSPlugin->registerModSamples(mod_samples);
  pthread_mutex_unlock(&plugin_mutex);
  return TRUE;
}

static void cleanup(void)
{
  fprintf(stderr, "libxmmsplugin: cleanup(): %s\n", configSectionString);
  pthread_mutex_lock(&plugin_mutex);
  plugin_available = false;
  fprintf(stderr, "NOTICE: cleanup() during play may not be supported.\n");
  delete XMMSPlugin;
  XMMSPlugin = NULL;
  pthread_mutex_unlock(&plugin_mutex);
#ifndef __GNUC__
  plugin_fini();
#endif
}

// common plugin fields

static void about(void){ if(XMMSPlugin != NULL) XMMSPlugin->about(); }
static void configure(void){ if(XMMSPlugin != NULL) XMMSPlugin->configure(); }
static void dsp_start(gint * channels, gint * rate){ if(XMMSPlugin != NULL) XMMSPlugin->start(channels, rate); }
static void dsp_process(gfloat ** data, gint * samples){ if(XMMSPlugin != NULL) XMMSPlugin->process(data,samples); }
static void dsp_flush(){ if(XMMSPlugin != NULL) XMMSPlugin->flush(); }
static void dsp_finish(gfloat ** data, gint * samples){ if(XMMSPlugin != NULL) XMMSPlugin->finish(data,samples); }
#ifdef AUDACIOUS36
static void * make_config_widget(){ if(XMMSPlugin != NULL) return XMMSPlugin->make_config_widget(); }
#endif

extern "C" {
  LibXmmsPluginTable libXmmsPluginTable = {
    productString, init, cleanup, about, configure, dsp_start, dsp_process, dsp_flush, dsp_finish,
#ifdef AUDACIOUS36
    make_config_widget, about_text,
#endif
  };
}
