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

#include "fv3_config.h"
#include <string.h>
#include <gtk/gtk.h>
#include "so.h"

#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

extern LibXmmsPluginTable libXmmsPluginTable;
static LibXmmsPluginTable * ptable = NULL;
static char pProductString[128];
static char pAboutString[256];
static void __attribute__ ((constructor)) plugin_table_init(void)
{
  ptable = &libXmmsPluginTable;
  snprintf(pProductString, 127, "%s", ptable->name);
  snprintf(pAboutString, 255, "%s", ptable->about_string);
}

static void * make_config_widget(void){ if(ptable != NULL) return  ptable->make_config_widget(); }
static const PreferencesWidget widgets[] = { WidgetCustomGTK (make_config_widget) };
static const PluginPreferences prefs = {{widgets}};

class EffectPluginInstance : public EffectPlugin
{
private:
  static constexpr PluginInfo info = { N_(pProductString), PACKAGE, pAboutString, & prefs, };
  
public:
  constexpr EffectPluginInstance () : EffectPlugin (info, 0, true) {}

  virtual bool init () {
    if(ptable != NULL) ptable->init();
  }
  
  virtual void cleanup () {
    if(ptable != NULL) ptable->cleanup();
  }
  
  virtual void start (int & channels, int & rate) {
    if(ptable != NULL) ptable->start(&channels, &rate);
  }
  
  virtual Index<float> & process (Index<float> & data) {
    float * f = data.begin ();
    float ** interleaved_data = &f;
    gint samples = data.len();
    if(ptable != NULL) ptable->process(interleaved_data, &samples);
    return data;
  }

  virtual Index<float> & finish (Index<float> & data, bool end_of_playlist) {
    float * f = data.begin ();
    float ** interleaved_data = &f;
    gint samples = data.len();
    if(ptable != NULL) ptable->finish(interleaved_data, &samples);
    return data;
  }

  virtual bool flush (bool force){ if(ptable != NULL) ptable->flush(); }

  virtual void show_about_window () {
    if(ptable != NULL) ptable->about();
  }
};

__attribute__((visibility("default"))) EffectPluginInstance aud_plugin_instance;
