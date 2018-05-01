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

#include <string.h>
#include <gtk/gtk.h>
#include "so.h"

#include <audacious/plugin.h>
#include <audacious/misc.h>
#include <audacious/preferences.h>
#include <libaudgui/libaudgui-gtk.h>

extern LibXmmsPluginTable libXmmsPluginTable;
static LibXmmsPluginTable * ptable = NULL;
static char pProductString[128];
static void __attribute__ ((constructor)) plugin_table_init(void)
{
  ptable = &libXmmsPluginTable;
  snprintf(pProductString, 127, "%s", ptable->name);
}

static bool_t init(void){ if(ptable != NULL) ptable->init(); }
static void cleanup(void){ if(ptable != NULL) ptable->cleanup(); }
static void configure(void){ if(ptable != NULL) ptable->configure(); }
static void about(void){ if(ptable != NULL) ptable->about(); }
static void dsp_start(gint * channels, gint * rate){ if(ptable != NULL) ptable->start(channels, rate); }
static void dsp_process(gfloat ** data, gint * samples){ if(ptable != NULL) ptable->process(data,samples); }
static void dsp_flush(){ if(ptable != NULL) ptable->flush(); }
static void dsp_finish(gfloat ** data, gint * samples){ if(ptable != NULL) ptable->finish(data,samples); }

AUD_EFFECT_PLUGIN
(
 .name = pProductString,
 .init = init,
 .cleanup = cleanup,
 .about = about,
 .configure = configure,
 .start = dsp_start,
 .process = dsp_process,
 .finish = dsp_finish,
 )
