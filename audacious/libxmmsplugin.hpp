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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <vector>
#include <string>
#include <map>
#include <unistd.h>
#include <pthread.h>
#include <gtk/gtk.h>

#ifdef AUDACIOUS
#ifdef AUDACIOUS36
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <libaudgui/libaudgui.h>
#include <libaudgui/libaudgui-gtk.h>
#else
extern "C" {
#include <audacious/plugin.h>
#include <audacious/misc.h>
#include <audacious/preferences.h>
#include <libaudgui/libaudgui-gtk.h>
}
#endif
#endif

#ifdef JACK
#include "util.h"
#include "configdb.h"
#endif

#include <freeverb/slot.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <libsamplerate2/samplerate2.h>

#define DELIMITER  "/"
#define KEY_CONFIG_WRITE "keyConfigWrite"

#ifdef PLUGDOUBLE
typedef double pfloat_t;
typedef fv3::slot_ SLOTP;
#else
typedef float pfloat_t;
typedef fv3::slot_f SLOTP;
#endif

enum { kFloat, kLong, kBool, kSelect, };
typedef void (*callbackFloat)(pfloat_t);
typedef void (*callbackLong)(long);
typedef void (*callbackBool)(gboolean);
typedef void (*callbackSelect)(long);
typedef struct
{
  void * self;
  std::string parameterDisplayName, parameterConfigName, parameterLabel, parameterSelect;
  pfloat_t minParam, maxParam;
  long displayDigit;
  pfloat_t valueF, defaultValueF;
  long valueL, defaultValueL;
  gboolean valueB, defaultValueB, isRTParameter, needNRTCall;
  unsigned typeOfParameter;
  void * callBack;
  GtkAdjustment * gtkAdjustment;
  GtkWidget * gtkWidget;
  std::vector<std::string> menuStrings;
} PluginParameter;
typedef struct
{
  const char *parameterDisplayName, *parameterConfigName, *parameterLabel, *parameterSelect;
  const pfloat_t minParam, maxParam; const long displayDigit;
  const pfloat_t defaultValueF;
  const long defaultValueL;
  const gboolean defaultValueB, isRTParameter, needNRTCall;
  const unsigned typeOfParameter;
  void * callBack;
} PluginParameterTable;

namespace fv3
{
  class libxmmsplugin
  {
  public:
    libxmmsplugin(const PluginParameterTable * confTable, const unsigned tableSize,
		  const char * aboutStr, const char * productStr, const char * configSectionStr)
    {
      this->aboutString = aboutStr;
      this->productString = productStr;
      this->configSectionString = configSectionStr;
      initConfTable(confTable, tableSize);
      for(unsigned i = 0;i < ParamsV.size();i ++)
	{
	  PluginParameter * pp = &ParamsV[i];
	  gboolean result = readConfig(pp, configSectionString);
	  if(result == false)
	    {
	      fprintf(stderr, "libxmmsplugin<%s>:: falling back to default preset.\n", configSectionString);
	      setDefault(pp);
	    }
	  if(pp->isRTParameter == true)
	    execCallBack(pp);
	  else
	    pp->needNRTCall = true;
	}
      conf_dialog = NULL;
      _mod_samples = NULL;
      plugin_rate = plugin_ch = 0;
    }
    
    ~libxmmsplugin()
    {
      if(conf_dialog != NULL) gtk_widget_destroy(GTK_WIDGET(conf_dialog));
    }

    void registerModSamples(void (*_vf)(pfloat_t * iL, pfloat_t * iR, pfloat_t * oL, pfloat_t * oR, gint length, gint srate))
    {
      _mod_samples = _vf;
    }

    void mod_samples(gfloat * LR, gint samples, gint srate)
    {
      if(_mod_samples == NULL) return;
      if(orig.getsize() < samples)
	{
	  orig.alloc(samples, 2);
	  reverb.alloc(samples, 2);
	}
#ifdef PLUGDOUBLE
      for(int tmpi = 0;tmpi < samples;tmpi ++)
        {
          orig.L[tmpi] = LR[tmpi*2+0];
          orig.R[tmpi] = LR[tmpi*2+1];
        }
#else
      fv3::splitChannelsV(2, samples, LR, orig.L, orig.R);
#endif
      _mod_samples(orig.L,orig.R,reverb.L,reverb.R,samples,srate);
#ifdef PLUGDOUBLE
      for(int tmpi = 0;tmpi < samples;tmpi ++)
        {
          LR[tmpi*2+0] = reverb.L[tmpi];
          LR[tmpi*2+1] = reverb.R[tmpi];
        }
#else
      fv3::mergeChannelsV(2, samples, LR, reverb.L, reverb.R);
#endif
    }

    void start(gint * channels, gint * rate)
    {
      fprintf(stderr, "libxmmsplugin<%s>: start: Ch %d Fs %d\n", configSectionString, *channels, *rate);
      plugin_rate = *rate; plugin_ch = *channels;
    }
    
    void process(gfloat ** data, gint * samples)
    {
      if(plugin_rate <= 0||plugin_ch != 2) return;
      mod_samples(*data, *samples/plugin_ch, plugin_rate);
    }

    void flush()
    {
      fprintf(stderr, "libxmmsplugin<%s>: flush:\n", configSectionString);
    }
    
    void finish(gfloat ** data, gint * samples)
    {
      fprintf(stderr, "libxmmsplugin<%s>: finish:\n", configSectionString);
      process(data,samples);
    }
    
    gboolean callNRTParameters()
    {
      gboolean ret = false;
      for(unsigned i = 0;i < ParamsV.size();i ++)
	{
	  PluginParameter * pp = &ParamsV[i];
	  if(pp->isRTParameter == false&&pp->needNRTCall == true)
	    {
	      pp->needNRTCall = false;
	      execCallBack(pp);
	      ret = true;
	    }
	}
      return ret;
    }

    void about(void)
    {
      static GtkWidget *about_dialog = NULL;
      if (about_dialog != NULL) return;
      audgui_simple_message(&about_dialog, GTK_MESSAGE_INFO, (gchar*)"About Plugin", (gchar*)aboutString);
    }
    
    void * make_config_widget ()
    {
#ifdef AUDACIOUS36
      GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_widget_set_size_request (vbox, 900, 700);
#endif

      GtkWidget * scrolledWindow = gtk_scrolled_window_new (NULL, NULL);
      gtk_container_set_border_width(GTK_CONTAINER (scrolledWindow), 10);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

#ifndef AUDACIOUS36
      gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(conf_dialog))), scrolledWindow, TRUE, TRUE, 0);
#else
      gtk_widget_set_size_request (scrolledWindow, 900, 650);
      gtk_box_pack_start ((GtkBox *) vbox, scrolledWindow, 0, 0, 0);
#endif
            
      GtkWidget * table = gtk_table_new(ParamsV.size()+1, 5, FALSE);
      gtk_table_set_col_spacings(GTK_TABLE(table), 1);
      gtk_container_set_border_width(GTK_CONTAINER(table), 11);
      gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledWindow), table);
      
      // create left labels
      for(unsigned i = 0;i < ParamsV.size();i ++)
	{
	  GtkWidget * label = createGUILabel(&ParamsV[i]);
	  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	  gtk_table_attach(GTK_TABLE(table),label,0,1,i,i+1,GTK_FILL,GTK_FILL,0,0);
	}
      
      // create adjustment/checkbox/menu
      for(unsigned i = 0;i < ParamsV.size();i ++)
	{
	  GtkWidget * object = createGUIObject(&ParamsV[i]);
	  gtk_table_attach_defaults(GTK_TABLE(table),object,1,2,i,i+1);
	  setGUIObjectValue(&ParamsV[i]);
	}
      
      // register gui callback
      for(unsigned i = 0;i < ParamsV.size();i ++) registerGUICallback(ParamsV[i].gtkWidget, &ParamsV[i]);
      
      GtkWidget * bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
      gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
      gtk_box_set_spacing(GTK_BOX(bbox), 2);
#ifndef AUDACIOUS36
      gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(conf_dialog))), bbox, TRUE, TRUE, 0);
#else
      gtk_box_pack_start ((GtkBox *) vbox, bbox, 0, 0, 0); 
#endif
      
      GtkWidget * button = gtk_button_new_with_label("Save");
      gtk_widget_grab_focus(button);
      gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
      g_signal_connect(button, "clicked", G_CALLBACK(conf_save_cb), this);

#ifndef AUDACIOUS36
      button = gtk_button_new_with_label("Close");
      gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
      g_signal_connect(button, "clicked", G_CALLBACK(conf_close_cb), this);
#endif
      
      button = gtk_button_new_with_label("Reload");
      gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
      g_signal_connect(button, "clicked", G_CALLBACK(conf_reload_cb), this);
      
      button = gtk_button_new_with_label("Default");
      gtk_box_pack_start(GTK_BOX(bbox), button, TRUE, TRUE, 0);
      g_signal_connect(button, "clicked", G_CALLBACK(conf_default_cb), this);

#ifndef AUDACIOUS36
      return NULL;
#else
      return vbox;
#endif
    }
    
    void configure(void)
    {
      if (conf_dialog != NULL) return;
      conf_dialog = gtk_dialog_new();
      g_signal_connect(conf_dialog, "destroy", G_CALLBACK(gtk_widget_destroyed), &conf_dialog);
      gtk_window_set_title(GTK_WINDOW(conf_dialog), productString);
      gtk_window_set_default_size(GTK_WINDOW(conf_dialog), 900, 700);
      gtk_widget_set_size_request(conf_dialog, 900, 700);

      make_config_widget();
      gtk_widget_show(conf_dialog);
      gtk_window_set_position(GTK_WINDOW(conf_dialog), GTK_WIN_POS_CENTER); 
      gtk_widget_show_all(conf_dialog);
      gtk_window_present(GTK_WINDOW(conf_dialog));
    }
    
  private:    
    void initConfTable(const PluginParameterTable * confTable, const unsigned tableSize)
    {
      ParamsV.clear();
      for(unsigned i = 0;i < tableSize;i ++)
	{
	  PluginParameter pp;
	  const PluginParameterTable * conft = confTable+i;
	  pp.parameterDisplayName = conft->parameterDisplayName;
	  pp.parameterConfigName = conft->parameterConfigName;
	  pp.parameterLabel = conft->parameterLabel;
	  pp.parameterSelect = conft->parameterSelect;
	  pp.minParam = conft->minParam; pp.maxParam = conft->maxParam;
	  pp.displayDigit = conft->displayDigit;
	  pp.valueF = pp.defaultValueF = conft->defaultValueF;
	  pp.valueL = pp.defaultValueL = conft->defaultValueL;
	  pp.valueB = pp.defaultValueB = conft->defaultValueB;
	  pp.isRTParameter = conft->isRTParameter;
	  pp.needNRTCall = conft->needNRTCall;
	  pp.typeOfParameter = conft->typeOfParameter;
	  pp.callBack = conft->callBack;
	  ParamsV.push_back(pp);
	  ParamsV.at(i).self = &ParamsV.at(i);
	}
    }
    
    static long splitStringToVector(std::string * s, std::vector<std::string> * v, const char * delimiter)
    {
      long count;
      v->clear();
      char * token = strtok((char*)s->c_str(), delimiter);
      while (token != NULL)
	{
	  v->push_back(std::string(token));
	  token = strtok(NULL, DELIMITER);
	  count++;
	}
      return count;
    }

    static void setDefault(PluginParameter * pp)
    {
      switch(pp->typeOfParameter)
	{
	case kFloat:
	  pp->valueF = pp->defaultValueF;
	  break;
	case kLong:
	case kSelect:
	  pp->valueL = pp->defaultValueL;
	  break;
	case kBool:
	  pp->valueB = pp->defaultValueB;
	  break;
	default:
	  break;
	}
    }
    
    static gboolean readConfig(PluginParameter * pp, const char * psName)
    {
      switch(pp->typeOfParameter)
	{
	case kFloat:
	  pp->valueF = aud_get_double((gchar*)psName, (gchar*)pp->parameterConfigName.c_str());
	  break;
	case kLong:
	case kSelect:
	  pp->valueL = aud_get_int((gchar*)psName, (gchar*)pp->parameterConfigName.c_str());
	  break;
	case kBool:
	  pp->valueB = aud_get_bool((gchar*)psName, (gchar*)pp->parameterConfigName.c_str());
	  break;
	default:
	  break;
	}
      return aud_get_bool((gchar*)psName, KEY_CONFIG_WRITE);
    }

    static void execCallBack(PluginParameter * pp)
    {
      switch(pp->typeOfParameter)
	{
	case kFloat:
	  ((callbackFloat)pp->callBack)(pp->valueF);
	  break;
	case kLong:
	case kSelect:
	  ((callbackLong)pp->callBack)(pp->valueL);
	  break;
	case kBool:
	  ((callbackBool)pp->callBack)(pp->valueB);
	  break;
	default:
	  break;
	}
    }
    
    static void writeConfig(PluginParameter * pp, const char * psName)
    {
      switch(pp->typeOfParameter)
	{
	case kFloat:
	  aud_set_double((gchar*)psName, (gchar*)pp->parameterConfigName.c_str(), pp->valueF);
	  break;
	case kLong:
	case kSelect:
	  aud_set_int((gchar*)psName, (gchar*)pp->parameterConfigName.c_str(), (gint)pp->valueL);
	  break;
	case kBool:
	  aud_set_bool((gchar*)psName, (gchar*)pp->parameterConfigName.c_str(), pp->valueB);
	  break;
	default:
	  break;
	}
      aud_set_bool((gchar*)psName, KEY_CONFIG_WRITE, true);
    }
    
    static GtkWidget * createGUILabel(PluginParameter * pp)
    {
      GtkWidget * go = NULL;
      std::string labelString = pp->parameterDisplayName + std::string(" ") + pp->parameterLabel;
      go = gtk_label_new(labelString.c_str());
      return go;
    }
    
    static GtkWidget * createGUIObject(PluginParameter * pp)
    {
      GtkWidget * go = NULL;
      switch(pp->typeOfParameter)
	{
	case kFloat:
	case kLong:
	  pp->gtkAdjustment = gtk_adjustment_new(pp->minParam, pp->minParam, pp->maxParam+1.0, 0.01, 0.1, 1.0);
	  pp->gtkWidget = go = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, pp->gtkAdjustment);
	  gtk_widget_set_size_request(go, 400, 35);
	  gtk_scale_set_digits(GTK_SCALE(go), (gint)pp->displayDigit);
	  break;
	case kBool:
	  pp->gtkWidget = go = gtk_check_button_new();
	  break;
	case kSelect:
	  // run once
	  if(pp->menuStrings.size() == 0) splitStringToVector(&pp->parameterSelect, &pp->menuStrings, DELIMITER);
	  GtkWidget *omenu = gtk_combo_box_text_new();
	  for(unsigned i = 0;i < pp->menuStrings.size();i ++)
	    {
	      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(omenu), NULL, pp->menuStrings[i].c_str());
	    }
	  pp->gtkWidget = go = GTK_WIDGET(omenu);
	  break;
	}
      return go;
    }
    
    static void setGUIObjectValue(PluginParameter * pp)
    {
      switch(pp->typeOfParameter)
	{
	case kFloat:
	  gtk_adjustment_set_value(pp->gtkAdjustment, pp->valueF);
	  break;
	case kLong:
	  gtk_adjustment_set_value(pp->gtkAdjustment, pp->valueL);
	  break;
	case kBool:
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pp->gtkWidget), pp->valueB);
	  break;
	case kSelect:
	  gtk_combo_box_set_active(GTK_COMBO_BOX(pp->gtkWidget), (gint)pp->valueL);
	  break;
	}
    }
    
    static void registerGUICallback(GtkWidget * go, PluginParameter * pp)
    {
      switch(pp->typeOfParameter)
	{
	case kFloat:
	case kLong:
	  // GtkAdjustment
	  g_signal_connect(pp->gtkAdjustment, "value_changed", G_CALLBACK(guiCallBack), pp);
	  break;
	case kBool:
	  // GtkCheckButton
	  g_signal_connect(go, "toggled", G_CALLBACK(guiCallBack), pp);
	  break;
	case kSelect:
	  // GtkComboBox
	  g_signal_connect(go, "changed", G_CALLBACK(guiCallBack), pp);
	  break;
	}
    }

    static void guiCallBack(GtkWidget * go, gpointer data)
    {
      PluginParameter * pp = (PluginParameter*)data;
      switch(pp->typeOfParameter)
	{
	case kFloat:  pp->valueF = gtk_adjustment_get_value(GTK_ADJUSTMENT(go)); break;
	case kLong:   pp->valueL = (long)gtk_adjustment_get_value(GTK_ADJUSTMENT(go)); break;
	case kBool:   pp->valueB = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(go)); break;
	case kSelect: pp->valueL = (long)gtk_combo_box_get_active(GTK_COMBO_BOX(go)); break;
	default: break;
	}
      if(pp->isRTParameter == true) execCallBack(pp); else pp->needNRTCall = true;
    }
    
    static void conf_save_cb(GtkButton * b, gpointer d)
    {
      if(d == NULL) return;
      libxmmsplugin * cl = (libxmmsplugin*)d;
      fprintf(stderr, "libxmmsplugin<%s>: save:\n", cl->configSectionString);
      for(unsigned i = 0;i < cl->ParamsV.size();i ++)
	{
	  PluginParameter * pp = &(cl->ParamsV[i]);
	  writeConfig(pp, cl->configSectionString);
	}
    }

    static void conf_close_cb(GtkButton * b, gpointer d)
    {
      if(d == NULL) return;
      libxmmsplugin * cl = (libxmmsplugin*)d;
      fprintf(stderr, "libxmmsplugin<%s>: close:\n", cl->configSectionString);
      gtk_widget_destroy(GTK_WIDGET(cl->conf_dialog));
      cl->conf_dialog = NULL;
    }
    
    static void conf_reload_cb(GtkButton * b, gpointer d)
    {
      if(d == NULL) return;
      libxmmsplugin * cl = (libxmmsplugin*)d;
      fprintf(stderr, "libxmmsplugin<%s>: reload:\n", cl->configSectionString);
      for(unsigned i = 0;i < cl->ParamsV.size();i ++)
	{
	  PluginParameter * pp = &(cl->ParamsV[i]);
	  gboolean result = readConfig(pp, cl->configSectionString);
	  if(result == true)
	    setGUIObjectValue(pp);
	  else
	    setDefault(pp);
	  if(pp->isRTParameter == true)
	    execCallBack(pp);
	  else
	    pp->needNRTCall = true;
	}
    }
    
    static void conf_default_cb(GtkButton * b, gpointer d)
    {
      if(d == NULL) return;
      libxmmsplugin * cl = (libxmmsplugin*)d;
      fprintf(stderr, "libxmmsplugin<%s>: default:\n", cl->configSectionString);
      for(unsigned i = 0;i < cl->ParamsV.size();i ++)
	{
	  PluginParameter * pp = &(cl->ParamsV[i]);
	  setDefault(pp);
	  setGUIObjectValue(pp);
	  if(pp->isRTParameter == true)
	    execCallBack(pp);
	  else
	    pp->needNRTCall = true;
	}
    }
    
    SLOTP origLR, orig, reverb;
    gint plugin_rate, plugin_ch;
    void (*_mod_samples)(pfloat_t * iL, pfloat_t * iR, pfloat_t * oL, pfloat_t * oR, gint length, gint srate);
    const char *aboutString, *productString, *configSectionString;
    std::vector<PluginParameter> ParamsV;
    GtkWidget *conf_dialog;
  };
};
