/**
 *  Impulse Response Processor XMMS plugin
 *  Low Latency Version
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
#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libsamplerate2/samplerate2.h>
#include <sndfile.h>
#include <freeverb/irmodel1.hpp>
#include <freeverb/irmodel2.hpp>
#include <freeverb/irmodel2zl.hpp>
#include <freeverb/irmodel3.hpp>
#ifdef ENABLE_PTHREAD
#include <freeverb/irmodel3p.hpp>
#endif
#include <freeverb/utils.hpp>
#include <freeverb/slot.hpp>
#include <freeverb/fv3_ch_tool.hpp>
#include <gdither.h>
#include "CFileLoader.hpp"

#include "so.h"

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
#include "plugin.h"
#include "util.h"
#include "configdb.h"
#endif

#define KEY_CONFIG_WRITE "keyConfigWrite"

#ifdef PLUGDOUBLE
typedef fv3::irbase_ IRBASE;
typedef fv3::irmodel1_ IRMODEL1;
typedef fv3::irmodel2_ IRMODEL2;
typedef fv3::irmodel2zl_ IRMODEL2ZL;
typedef fv3::irmodel3_ IRMODEL3;
#ifdef ENABLE_PTHREAD
typedef fv3::irmodel3p_ IRMODEL3P;
#endif
typedef fv3::utils_ UTILS;
typedef double pfloat_t;
typedef fv3::CFileLoader_ CFILELOADER;
typedef fv3::slot_ SLOTP;
#else
typedef fv3::irbase_f IRBASE;
typedef fv3::irmodel1_f IRMODEL1;
typedef fv3::irmodel2_f IRMODEL2;
typedef fv3::irmodel2zl_f IRMODEL2ZL;
typedef fv3::irmodel3_f IRMODEL3;
#ifdef ENABLE_PTHREAD
typedef fv3::irmodel3p_f IRMODEL3P;
#endif
typedef fv3::utils_f UTILS;
typedef float pfloat_t;
typedef fv3::CFileLoader_f CFILELOADER;
typedef fv3::slot_f SLOTP;
#endif

class ReverbVector
{
public:
  ReverbVector(){ rv.clear(); }
  ~ReverbVector(){ clear(); }
  IRBASE * at(unsigned int i){ return rv[i]; }
  IRBASE * operator[](unsigned int i){ return rv[i]; }
  IRBASE * assign(unsigned int i, const char * type)
  {
    delete rv.at(i);
    rv[i] = new_model(type);
    return rv[i];
  }
  IRBASE * push_back(const char * type)
  {
    IRBASE * model = new_model(type);
    rv.push_back(model);
    return model;
  }
  void pop_back()
  {
    delete rv[size()-1];
    rv.pop_back();
  }
  void clear()
  {
    for(std::vector<IRBASE*>::iterator i = rv.begin();i != rv.end();i ++) delete *i;
    rv.clear();
  }
  unsigned int size(){ return rv.size(); }
private:
  std::vector<IRBASE*> rv;
  IRBASE * new_model(const char * type)
  {
    IRBASE * model = NULL;
    if(strcmp(type, "irmodel1") == 0)    model = new IRMODEL1;
    if(strcmp(type, "irmodel2") == 0)   model = new IRMODEL2;
    if(strcmp(type, "irmodel2zl") == 0) model = new IRMODEL2ZL;
    if(strcmp(type, "irmodel3") == 0)   model = new IRMODEL3;
#ifdef ENABLE_PTHREAD
    if(strcmp(type, "irmodel3p") == 0)  model = new IRMODEL3P;
#else
    if(strcmp(type, "irmodel3p") == 0)  model = new IRMODEL3;
#endif
    if(model == NULL) model = new IRMODEL2;
    return model;
  }
};

static const char *about_text = 
  "Freeverb3 "VERSION"\n"
  "Impulse Response Processor V2\n"
  "XMMS / BMP / Audacious / JACK Plugin\n"
#ifdef PLUGDOUBLE
  "Double Precision Version\n"
#else
  "Single Precision Version\n"
#endif
  "Copyright (C) 2006-2018 Teru Kamogashira\n"
  "http://www.nongnu.org/freeverb3/";

static const char *productString = "Freeverb3 "VERSION" [Impulser V2]";
static const char *configSectionString = "freeverb3_plugin_irmodel2";

static bool validModel = false;
static int StreamFs = 0;
// Latency
static int latencyIndex = 0, conf_latency_index = 4;
static const int presetLatencyMax = 7;
static const char * presetLatencyString[] =
  {"1024|512x8", "2048|512x16", "4096|512x32", "8192|1024x8", "16384|1024x16(Default)", "32768|1024x32", "65536|2048x8",};
static const long presetLatencyValue[] =  {1024, 2048, 4096, 8192, 16384, 32768, 65536,};
static const long presetLatencyValue1[] = {512, 512, 512, 1024, 1024, 1024, 2048,};
static const long presetLatencyValue2[] = {8,   16,  32,  8,    16,   32,   8,};

// mono/stereo/swap slot
static const int presetSlotModeMax = 3;
static const char * presetSlotModeString[] = {"Mono((L+R)/2) =>> Stereo", "Stereo =>> Stereo", "Swap LR",};
static const int presetSlotModeValue[] = {1, 0, 3,};

// dithering
static const int presetDitherMax = 4;
static const char * presetDitherString[] = {"GDitherNone", "GDitherRect", "GDitherTri", "GDitherShaped"};
static const GDitherType presetDitherValue[] = {GDitherNone, GDitherRect, GDitherTri, GDitherShaped,};
static int conf_dithering = -1, next_dithering = 3;
static GDither pdither;
static gboolean gdither_on = FALSE;

static const int presetIRModelMax = 5;
static const char * presetIRModelString[] =
  { "fv3::irmodel2", "fv3::irmodel3 (Zero Latency)", "fv3::irmodel2zl (Zero Latency)", "fv3::irmodel1", "fv3::irmodel3p (Zero Latency|Pthread)",};
static const char * presetIRModelValue[] =
  {"irmodel2", "irmodel3", "irmodel2zl", "irmodel1", "irmodel3p"};
static int conf_rev_zl = 1;

// SLOT1 DRY + WET
// SLOT2~ WET only

typedef struct{
  float wet, dry, lpf, hpf, width, stretch, limit, idelay;
  int i1o2_index, valid;
  std::string filename, inf;
} SlotConfiguration;

#define SLOT_MAX 32
static int slotNumber = 1, currentSlot = 1;

// These should be initialized in init() and
// cleaned in cleanup()
static std::vector<SlotConfiguration> * slotVector = NULL;
// ... and only mod_samples() operate these vectors
static std::vector<SlotConfiguration> * currentSlotVector = NULL;
static ReverbVector * reverbVector = NULL;
static pthread_mutex_t plugin_mutex;
static gboolean plugin_available = false;
#define MAX_KEY_STR_LENGTH 1024
static char key_i_string[MAX_KEY_STR_LENGTH];
static const char * key_i(const char * str, int i)
{
  if(i == 1) return str;
  else { sprintf(key_i_string, "%s__%d", str, i); return key_i_string; }
}

static void slot_init(SlotConfiguration * slot)
{
#ifdef DEBUG
  fprintf(stderr, "Impulser2: slot_init\n");
#endif
  slot->wet = -28.0f;
  slot->lpf = 0.0f;
  slot->hpf = 0.0f;
  slot->width = 1.0f;
  slot->stretch = 0.0f;
  slot->limit = 100.0f;
  slot->idelay = 0.0f;
  slot->i1o2_index = 1;
  slot->valid = 0;
}

static void slot_save(SlotConfiguration * slot, int i)
{
  aud_set_double(configSectionString, key_i("wet",i), slot->wet);
  aud_set_double(configSectionString, key_i("dry",i), slot->dry);
  aud_set_double(configSectionString, key_i("width",i), slot->width);
  aud_set_double(configSectionString, key_i("LPF",i), slot->lpf);
  aud_set_double(configSectionString, key_i("HPF",i), slot->hpf);
  aud_set_double(configSectionString, key_i("stretch",i), slot->stretch);
  aud_set_double(configSectionString, key_i("limit",i), slot->limit);
  aud_set_double(configSectionString, key_i("idelay",i), slot->idelay);
  aud_set_int   (configSectionString, key_i("i1o2_index",i), slot->i1o2_index);
  aud_set_str   (configSectionString, key_i("file",i), (gchar*)slot->filename.c_str());
}

static void slot_load(SlotConfiguration * slot, int i)
{
  gchar * filename;
  gchar blank[] = "";
  slot->wet =   aud_get_double (configSectionString, key_i("wet",i));
  slot->dry =   aud_get_double (configSectionString, key_i("dry",i));
  slot->width = aud_get_double (configSectionString, key_i("width",i));
  slot->lpf =   aud_get_double (configSectionString, key_i("LPF",i));
  slot->hpf =   aud_get_double (configSectionString, key_i("HPF",i));
  slot->stretch = aud_get_double (configSectionString, key_i("stretch",i));
  slot->limit =   aud_get_double (configSectionString, key_i("limit",i));
  slot->idelay =  aud_get_double (configSectionString, key_i("idelay",i));
  slot->i1o2_index = aud_get_int (configSectionString, key_i("i1o2_index",i));
#if   defined(AUDACIOUS39) && !defined(JACK)
  filename = (gchar*)((const char*)aud_get_str(configSectionString, key_i("file",i)));
#elif defined(AUDACIOUS36) && !defined(JACK)
  filename = aud_get_str(configSectionString, key_i("file",i)).to_raw();
#else // audacious-3.5 or jack
  filename = aud_get_str(configSectionString, key_i("file",i));
#endif
  if(filename == NULL)
    {
      filename = blank;
    }
  slot->filename = filename;
  if(std::string("") == slot->filename) slot_init(slot);
}

static int protectValue = 0;

static void set_rt_reverb(IRBASE * reverbm, SlotConfiguration * slot)
{
  reverbm->setwet(slot->wet);
  reverbm->setdry(0);
  reverbm->setLPF(slot->lpf);
  reverbm->setHPF(slot->hpf);
  reverbm->setwidth(slot->width);
  if(reverbVector->size() > 0&&slotVector->size() > 0) (*reverbVector)[0]->setdry((*slotVector)[0].dry);
}

// libsndfile

static char inf[1024] = "";

static int store_inf(const char * file)
{
  SF_INFO rsfinfo;
  SNDFILE * sndFile = sf_open(file, SFM_READ, &rsfinfo);
  if(sndFile == NULL)
    {
      fprintf(stderr, "Impulser2: store_inf: sf_open: Couldn't load \"%s\"\n", file);
      return -1;
    }
  float second = (float)rsfinfo.frames/(float)rsfinfo.samplerate;
  sprintf(inf, "%lld[samples] %d[Hz] %d[Ch] %f[s]",
          (long long int)rsfinfo.frames, rsfinfo.samplerate, rsfinfo.channels, second);
  sf_close(sndFile);
  return 0;
}

// Logo
#include "wave.xpm"

// Splash
static GtkWidget *splashWindow = NULL;

static void hideSplash()
{
  if(splashWindow == NULL) return;
  gtk_widget_destroy(GTK_WIDGET(splashWindow));
  splashWindow = NULL;
}

static void showSplash(const char * c1, const char * c2, const char * c3)
{
  if(splashWindow != NULL) return;
  splashWindow = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_widget_realize(splashWindow);

  GtkWidget *logo = NULL, *splashTable = NULL, *splashLabel1 = NULL, *splashLabel2 = NULL, *splashLabel3 = NULL, *button = NULL;
  GdkPixbuf *pixbuf = NULL;
  pixbuf = gdk_pixbuf_new_from_xpm_data(wave_xpm);
  logo = gtk_image_new_from_pixbuf(pixbuf);

  splashLabel1 = gtk_label_new(c1);
  splashLabel2 = gtk_label_new(c2);
  splashLabel3 = gtk_label_new(c3);
  splashTable = gtk_table_new(5, 5, FALSE);
  gtk_table_attach(GTK_TABLE(splashTable), logo,         0,5, 0,1, GTK_FILL, GTK_FILL, 0,0);  
  gtk_table_attach(GTK_TABLE(splashTable), splashLabel1, 1,4, 1,2, GTK_FILL, GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(splashTable), splashLabel2, 1,4, 2,3, GTK_FILL, GTK_FILL, 0,0);
  gtk_table_attach(GTK_TABLE(splashTable), splashLabel3, 1,4, 3,4, GTK_FILL, GTK_FILL, 0,0);
  
  button = gtk_button_new_with_label("Close");
  g_signal_connect(button, "clicked", G_CALLBACK(hideSplash), NULL);
  gtk_table_attach(GTK_TABLE(splashTable), button, 2,3, 5,6, GTK_FILL, GTK_FILL, 0,0);
  gtk_container_add(GTK_CONTAINER(splashWindow), splashTable);
  gtk_container_set_border_width(GTK_CONTAINER(splashWindow), 10);
  gtk_window_set_position(GTK_WINDOW(splashWindow), GTK_WIN_POS_CENTER); 
  gtk_widget_show_all(splashWindow);
  gtk_window_present(GTK_WINDOW(splashWindow));
}

static void about(void)
{
  showSplash("", about_text, "");
}

static GtkWidget *show_filename = NULL, *show_inf = NULL;

// file selector

static void select_file(GtkWindow *parent)
{
  std::string fc_filename;
  GtkWidget *file_dialog = gtk_file_chooser_dialog_new("Please select a Impulse Response wav file.", parent,
                                                       GTK_FILE_CHOOSER_ACTION_OPEN,
                                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                       GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, (char*)NULL);

  gtk_widget_set_size_request(file_dialog, 900, 700);
  gtk_window_set_position(GTK_WINDOW(file_dialog), GTK_WIN_POS_CENTER);
  if(gtk_dialog_run(GTK_DIALOG(file_dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *filename;
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_dialog));
      fc_filename = std::string(filename);
      g_free (filename);
    }
  gtk_widget_destroy(file_dialog);
  if(fc_filename == std::string("")) return;
  if(store_inf(fc_filename.c_str()) != 0) // check the sound file
    {
      GtkWidget * err_dialog = NULL;
      audgui_simple_message(&err_dialog, GTK_MESSAGE_ERROR, (gchar*)"Impulser2 Error", (gchar*)"Could not load IR file.");
      return;
    }
  (*slotVector)[currentSlot-1].filename = fc_filename;
  (*slotVector)[currentSlot-1].inf = inf;
  gtk_label_set_text(GTK_LABEL(show_filename), fc_filename.c_str());
  gtk_label_set_text(GTK_LABEL(show_inf), inf);
}

// GUI

static GtkWidget *conf_rev_dialog = NULL;
static GtkAdjustment *conf_rev_wet_adj, *conf_rev_dry_adj, *conf_rev_lpf_adj, *conf_rev_hpf_adj,
  *conf_rev_width_adj, *conf_rev_stretch_adj, *conf_rev_limit_adj, *conf_rev_idelay_adj;
// slots
static GtkAdjustment *slotAdjustment;
static GtkWidget *slotWidget, *slotLabel, *optionMenu_MSOption;

static void slot_show(SlotConfiguration * slot)
{
  gtk_adjustment_set_value(conf_rev_wet_adj,   slot->wet);
  gtk_adjustment_set_value(conf_rev_lpf_adj,   slot->lpf);
  gtk_adjustment_set_value(conf_rev_hpf_adj,   slot->hpf);
  gtk_adjustment_set_value(conf_rev_width_adj, slot->width);
  gtk_adjustment_set_value(conf_rev_stretch_adj, slot->stretch);
  gtk_adjustment_set_value(conf_rev_limit_adj,   slot->limit);
  gtk_adjustment_set_value(conf_rev_idelay_adj,  slot->idelay);
  gtk_combo_box_set_active(GTK_COMBO_BOX(optionMenu_MSOption), (gint)slot->i1o2_index);
  gtk_label_set_text(GTK_LABEL(show_filename), slot->filename.c_str());
  gtk_label_set_text(GTK_LABEL(show_inf), slot->inf.c_str());
}

static void write_config(void)
{
  aud_set_int(configSectionString, "latency_index", conf_latency_index);
  aud_set_int(configSectionString, "dithering_mode", conf_dithering);
  aud_set_int(configSectionString, "zero_latency", conf_rev_zl);
  aud_set_int(configSectionString, "slotNumber", slotNumber);
  for(int i = 0;i < slotNumber;i ++) slot_save(&(*slotVector)[i], i+1);
}

static GtkWidget *applyButton = NULL;

// apply changes immediately if RT parameters were changed
static void conf_apply_realtime_sig(GtkButton * button, gpointer data)
{
  if(protectValue == 0)
    {
      (*slotVector)[currentSlot-1].wet = gtk_adjustment_get_value(conf_rev_wet_adj);
      (*slotVector)[currentSlot-1].lpf = gtk_adjustment_get_value(conf_rev_lpf_adj);
      (*slotVector)[currentSlot-1].hpf = gtk_adjustment_get_value(conf_rev_hpf_adj);
      (*slotVector)[currentSlot-1].width = gtk_adjustment_get_value(conf_rev_width_adj);
      (*slotVector)[0].dry = gtk_adjustment_get_value(conf_rev_dry_adj);
      if(currentSlot <= (int)reverbVector->size()) set_rt_reverb((*reverbVector)[currentSlot-1], &(*slotVector)[currentSlot-1]);
    }
}

// enable apply button if non-RT parameters were changed
static void conf_non_realtime_sig(GtkButton * button, gpointer data)
{
  if(protectValue == 0)
    {
      if(applyButton != NULL) gtk_widget_set_sensitive(applyButton, TRUE);
    }
}

// Apply non-RT parameters
static void conf_rev_apply_cb(GtkButton * button, gpointer data)
{
  if(currentSlot <= (int)slotVector->size())
    {
      (*slotVector)[currentSlot-1].stretch = gtk_adjustment_get_value(conf_rev_stretch_adj);
      (*slotVector)[currentSlot-1].limit = gtk_adjustment_get_value(conf_rev_limit_adj);
      (*slotVector)[currentSlot-1].idelay = gtk_adjustment_get_value(conf_rev_idelay_adj);
      if(applyButton != NULL) gtk_widget_set_sensitive(applyButton, FALSE);
    }
}

static void conf_rev_default_cb(GtkButton * button, gpointer data)
{
  protectValue = 1;
  slot_init(&(*slotVector)[currentSlot-1]);
  slot_show(&(*slotVector)[currentSlot-1]);
  set_rt_reverb((*reverbVector)[currentSlot-1], &(*slotVector)[currentSlot-1]);
  protectValue = 0;
  if(applyButton != NULL) gtk_widget_set_sensitive(applyButton, FALSE);
}

static void conf_rev_select_cb(GtkButton * button, gpointer data)
{
  select_file((GtkWindow*)data);
}

static void conf_rev_ok_cb(GtkButton * button, gpointer data)
{
  conf_rev_apply_cb(button, data);
  write_config();
}

static void conf_rev_cancel_cb(GtkButton * button, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(conf_rev_dialog));
}

static void conf_set_i1o2(GtkWidget *go, gpointer data)
{
  if(protectValue != 0) return;
  gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(go));
  fprintf(stderr, "Impulser2: I1O2 %s(%d)\n", presetSlotModeString[select], select);
  (*slotVector)[currentSlot-1].i1o2_index = select;
}

static void conf_set_dithering(GtkWidget *go, gpointer data)
{
  gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(go));
  fprintf(stderr, "Impulser2: set_dithering(%d)=%s\n", select, presetDitherString[GPOINTER_TO_INT(select)]);
  next_dithering = select;
}

static void conf_set_latency(GtkWidget *go, gpointer data)
{
  gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(go));
  fprintf(stderr, "Impulser2: set_latency(%d)=%s\n", select, presetLatencyString[GPOINTER_TO_INT(select)]);
  conf_latency_index = select;
}

static void conf_set_irmodel(GtkWidget *go, gpointer data)
{
  gint select = gtk_combo_box_get_active(GTK_COMBO_BOX(go));
  fprintf(stderr, "Impulser2: set_irmodel(%d)[%s]<%s>\n", select, presetIRModelString[select], presetIRModelValue[select]);
  if(conf_rev_zl != select) conf_rev_zl = select, StreamFs = 0; // reset all slot
}

static void conf_slot_inc_sig(GtkButton * button, gpointer data)
{
  fprintf(stderr, "Impulser2: WARNING: increasing slot during play is not supported!!\n");
  if(slotNumber >= SLOT_MAX) return;

  pthread_mutex_lock(&plugin_mutex);

  std::ostringstream os;
  os << slotNumber+1;
  gtk_label_set_text(GTK_LABEL(slotLabel), os.str().c_str());
  gint current = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(slotWidget));
  slotAdjustment = gtk_adjustment_new(current, 1, slotNumber+1, 1, 1, 0);
  gtk_spin_button_configure(GTK_SPIN_BUTTON(slotWidget), GTK_ADJUSTMENT(slotAdjustment),1,0);
  
  SlotConfiguration slotC;
  slot_load(&slotC, slotNumber+1);
  if(store_inf(slotC.filename.c_str()) != 0) sprintf(inf, "(not loaded)");
  slotC.inf = inf;
  slotVector->push_back(slotC);
  
  slotNumber ++;
  fprintf(stderr, "Impulser2: slot_inc: (*slotVector)[%d]\n", (int)slotVector->size());

  pthread_mutex_unlock(&plugin_mutex);
}

static void conf_slot_dec_sig(GtkButton * button, gpointer data)
{
  fprintf(stderr, "Impulser2: WARNING: decreasing slot during play is not supported!!\n");
  if(slotNumber <= 1) return;

  pthread_mutex_lock(&plugin_mutex);

  std::ostringstream os;
  os << slotNumber-1;
  gtk_label_set_text(GTK_LABEL(slotLabel), os.str().c_str());
  
  slot_save(&(*slotVector)[slotVector->size()-1], slotVector->size());
  slotVector->pop_back();
  
  gint current = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(slotWidget));
  if(current > slotNumber-1) current = slotNumber-1;
  slotAdjustment = gtk_adjustment_new(current, 1, slotNumber-1, 1, 1, 0);
  gtk_spin_button_configure(GTK_SPIN_BUTTON(slotWidget), GTK_ADJUSTMENT(slotAdjustment),1,0);
  
  slotNumber --;
  fprintf(stderr, "Impulser2: slot_dec: (*slotVector)[%d]\n", (int)slotVector->size());

  pthread_mutex_unlock(&plugin_mutex);
}

static void conf_rev_slot_select_changed_sig(GtkSpinButton * button, gpointer data)
{
  conf_rev_apply_cb(NULL, NULL);
  gint value = gtk_spin_button_get_value_as_int(button);
#ifdef DEBUG
  fprintf(stderr, "Impulser2: UI slot %d -> %d/%ld\n", currentSlot, value, slotVector->size());
#endif
  currentSlot = value;
  protectValue = 1;
  if(currentSlot < 0) return;
  slot_show(&(*slotVector)[currentSlot-1]);
  protectValue = 0;
}

#define MIN_DB (-100.0)
#define MAX_DB (20.0)

static void * make_config_widget ()
{
  GtkWidget *button, *table, *label, *hscale, *bbox;
#ifdef AUDACIOUS36
  GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_size_request (vbox, 900, 700);
#endif
  
  conf_rev_wet_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].wet,      MIN_DB, MAX_DB+1.0, 0.01, 0.1, 1.0);
  conf_rev_dry_adj = gtk_adjustment_new((*slotVector)[0].dry,                  MIN_DB, MAX_DB+1.0, 0.01, 0.1, 1.0);
  conf_rev_lpf_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].lpf,         0.0, 1.0+1.0, 0.01, 0.1, 1.0);
  conf_rev_hpf_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].hpf,         0.0, 1.0+1.0, 0.01, 0.1, 1.0);
  conf_rev_width_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].width,     0.0, 1.0+1.0, 0.01, 0.1, 1.0);
  conf_rev_stretch_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].stretch,-6.0, 6.0+1.0, 0.01, 0.1, 1.0);
  conf_rev_limit_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].limit,     0.0, 100.0+1.0, 0.01, 0.1, 1.0);
  conf_rev_idelay_adj = gtk_adjustment_new((*slotVector)[currentSlot-1].idelay,-800.0, 800.0+1.0, 0.01, 0.1, 1.0);
  
#define LABELS 14 // size of labels
#define SHOWLABEL 6 // size of label which do not have gtk_adjustment
  const char * labels[64] = {"wet [dB]", "*dry [dB]", "1-pole LPF", "1-zero HPF",
                             "width", "stretch [sqrt(2)^]", "ir limit [%]", "initial delay [ms]",
                             "impulse file:", "pcm inf.", "mono/stereo slot",
                             "*fragment size (<irmodel2|>irmodel3)", "*ir model type", "*dithering mode",};
  
  table = gtk_table_new(LABELS, 5, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 1);
  gtk_container_set_border_width(GTK_CONTAINER(table), 1);

#ifndef AUDACIOUS36
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(conf_rev_dialog))), table, TRUE, TRUE, 1);
#else
  gtk_widget_set_size_request (table, 900, 650);
  gtk_box_pack_start ((GtkBox *) vbox, table, 0, 0, 0);
#endif
  
  // label
  for(int i = 0;i < LABELS;i ++)
    {
      label = gtk_label_new(labels[i]);
      gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
      gtk_table_attach(GTK_TABLE(table), label, 0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show(label);
    }
  
  GtkAdjustment * objects[] =
    {conf_rev_wet_adj, conf_rev_dry_adj, conf_rev_lpf_adj, conf_rev_hpf_adj, conf_rev_width_adj, conf_rev_stretch_adj, conf_rev_limit_adj, conf_rev_idelay_adj};
  for(int i = 0;i < LABELS-SHOWLABEL;i ++)
    {
      hscale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, objects[i]);
      gtk_widget_set_size_request(hscale, 300, 35);
      gtk_scale_set_digits(GTK_SCALE(hscale), 2); // under .
      gtk_table_attach_defaults(GTK_TABLE(table), hscale, 1, 5, i, i+1);
      gtk_widget_show(hscale);
    }
  // signal
  int isRealTimeObject[] = {1, 1, 1, 1, 1, 0, 0, 0,};
  for(int i = 0;i < LABELS-SHOWLABEL;i ++)
    {
      if(isRealTimeObject[i] == 1)
        g_signal_connect(objects[i], "value_changed", G_CALLBACK(conf_apply_realtime_sig), NULL);
      else
        g_signal_connect(objects[i], "value_changed", G_CALLBACK(conf_non_realtime_sig), NULL);
    }
  
  // filename + information
  show_filename = gtk_label_new((*slotVector)[currentSlot-1].filename.c_str());
  gtk_table_attach_defaults(GTK_TABLE(table), show_filename, 1, 5, LABELS-SHOWLABEL, LABELS-SHOWLABEL+1);
  gtk_widget_show(show_filename);
  show_inf = gtk_label_new((*slotVector)[currentSlot-1].inf.c_str());
  gtk_table_attach_defaults(GTK_TABLE(table), show_inf, 1, 5, LABELS-SHOWLABEL+1, LABELS-SHOWLABEL+2);
  gtk_widget_show(show_inf);

  // mono/stereo slot
  optionMenu_MSOption = gtk_combo_box_text_new();
  for(int i = 0;i < presetSlotModeMax;i ++)
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(optionMenu_MSOption), NULL, presetSlotModeString[i]);
  g_signal_connect(optionMenu_MSOption, "changed", G_CALLBACK(conf_set_i1o2), NULL);
  gtk_table_attach(GTK_TABLE(table), optionMenu_MSOption, 1, 2, LABELS-SHOWLABEL+2, LABELS-SHOWLABEL+3, GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(optionMenu_MSOption), (gint)(*slotVector)[currentSlot-1].i1o2_index);
  gtk_widget_show(optionMenu_MSOption);
  
  // latency/fragment size
  GtkWidget *optionMenu_LatencyOption = gtk_combo_box_text_new();
  for(int i = 0;i < presetLatencyMax;i ++)
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(optionMenu_LatencyOption), NULL, presetLatencyString[i]);
  g_signal_connect(optionMenu_LatencyOption, "changed", G_CALLBACK(conf_set_latency), NULL);
  gtk_table_attach(GTK_TABLE(table), optionMenu_LatencyOption, 1, 2, LABELS-SHOWLABEL+3, LABELS-SHOWLABEL+4, GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(optionMenu_LatencyOption), (gint)conf_latency_index);
  gtk_widget_show(optionMenu_LatencyOption);
  
  // ir model selection
  GtkWidget *optionMenu_IRModel = gtk_combo_box_text_new();
  for(int i = 0;i < presetIRModelMax;i ++)
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(optionMenu_IRModel), NULL, presetIRModelString[i]);
  g_signal_connect(optionMenu_IRModel, "changed", G_CALLBACK(conf_set_irmodel), NULL);
  gtk_table_attach(GTK_TABLE(table), (GtkWidget*)optionMenu_IRModel, 1, 2, LABELS-SHOWLABEL+4, LABELS-SHOWLABEL+5, GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(optionMenu_IRModel), (gint)conf_rev_zl);
  gtk_widget_show(optionMenu_IRModel);

  // dithering
  GtkWidget * omenu3 =  gtk_combo_box_text_new();
  for(int i = 0;i < presetDitherMax;i ++)
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(omenu3), NULL, presetDitherString[i]);
  g_signal_connect(omenu3, "changed", G_CALLBACK(conf_set_dithering), NULL);
  gtk_table_attach(GTK_TABLE(table), (GtkWidget*)omenu3, 1, 2, LABELS-SHOWLABEL+5, LABELS-SHOWLABEL+6, GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(omenu3), (gint)next_dithering);
  gtk_widget_show(omenu3);
  
  gtk_widget_show(table);

  button = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 5, LABELS-SHOWLABEL+6, LABELS-SHOWLABEL+7);
  gtk_widget_show(GTK_WIDGET(button));
  gtk_widget_show(button);
  
  // slots box
  bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 1);
  
  // buttons
  label = gtk_label_new("Slot:");
  gtk_box_pack_start(GTK_BOX(bbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  // slot selector
  slotAdjustment = gtk_adjustment_new(currentSlot, 1, slotNumber, 1, 1, 0);
  slotWidget = gtk_spin_button_new(GTK_ADJUSTMENT(slotAdjustment),1,0);
  gtk_box_pack_start(GTK_BOX(bbox), slotWidget, FALSE, FALSE, 0);
  g_signal_connect(slotWidget, "changed", G_CALLBACK(conf_rev_slot_select_changed_sig), NULL);
  gtk_widget_show(slotWidget);

  // slot numbers
  std::ostringstream os;
  os << slotNumber;
  slotLabel = gtk_label_new(os.str().c_str());
  gtk_box_pack_start(GTK_BOX(bbox), slotLabel, FALSE, FALSE, 0);
  gtk_widget_show(slotLabel);

  // slot +
  button = gtk_button_new_with_label("Slot +");
  //GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_slot_inc_sig), NULL);
  gtk_widget_show(button);

  // slot -
  button = gtk_button_new_with_label("Slot -");
  //GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_slot_dec_sig), NULL);
  gtk_widget_show(button);

  gtk_table_attach_defaults(GTK_TABLE(table), bbox, 0, 5, LABELS-SHOWLABEL+7, LABELS-SHOWLABEL+8);
  gtk_widget_show(GTK_WIDGET(bbox));
  gtk_widget_show(bbox);
  
  // ====

  // Controls
  bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(bbox), 1);
#ifndef AUDACIOUS36
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(conf_rev_dialog))), bbox, FALSE, FALSE, 0);
#else
  gtk_box_pack_start ((GtkBox *) vbox, bbox, 0, 0, 0); 
#endif

  button = gtk_button_new_with_label("Save");
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_rev_ok_cb), NULL);

#ifndef AUDACIOUS36
  button = gtk_button_new_with_label("Close");
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_rev_cancel_cb), NULL);
#endif
  
  applyButton = button = gtk_button_new_with_label("Apply");
  gtk_widget_set_sensitive(applyButton, FALSE);
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_rev_apply_cb), NULL);
  
  button = gtk_button_new_with_label("Default");
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_rev_default_cb), NULL);
  
  button = gtk_button_new_with_label("Load");
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(conf_rev_select_cb), gtk_widget_get_window(conf_rev_dialog));
  
  button = gtk_button_new_with_label("About");
  gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "clicked", G_CALLBACK(about), NULL);

#ifndef AUDACIOUS36
  return NULL;
#else
  return vbox;
#endif
}
 
static void configure(void)
{
  if(validModel != true) return;
  if(conf_rev_dialog != NULL) return;
  conf_rev_dialog = gtk_dialog_new();
  g_signal_connect(conf_rev_dialog, "destroy", G_CALLBACK(gtk_widget_destroyed), &conf_rev_dialog);
  gtk_window_set_title(GTK_WINDOW(conf_rev_dialog), productString);

  make_config_widget();
  
  gtk_window_set_position(GTK_WINDOW(conf_rev_dialog), GTK_WIN_POS_CENTER);
  gtk_widget_show_all(conf_rev_dialog);
}

// plugin functions
static void
#ifdef __GNUC__
__attribute__ ((constructor))
#endif
plugin_init(void)
{
  std::fprintf(stderr, "Impulser2: plugin_init()\n");
  pthread_mutex_init(&plugin_mutex, NULL);
}

static void
#ifdef __GNUC__
__attribute__ ((destructor))
#endif
plugin_fini(void)
{
  std::fprintf(stderr, "Impulser2: plugin_fini()\n");
  pthread_mutex_destroy(&plugin_mutex);
}

static gboolean init(void)
{
#ifndef __GNUC__
  plugin_init();
#endif
  fprintf(stderr, "Impulser2: init()\n");
  fprintf(stderr, "SIMDFlag: 0x%08x\n", UTILS::getSIMDFlag());
  if(UTILS::getSIMDFlag()&FV3_X86SIMD_FLAG_SSE)
    {
      fprintf(stderr, "MXCSRMASK: 0x%08x\n", UTILS::getMXCSR_MASK());
      fprintf(stderr, "MXCSR: 0x%08x\n", UTILS::getMXCSR());
    }

  pthread_mutex_lock(&plugin_mutex);
  plugin_available = true;

  if(validModel == false)
    {
      slotVector = new std::vector<SlotConfiguration>;
      currentSlotVector = new std::vector<SlotConfiguration>;
      reverbVector = new ReverbVector;
      validModel = true;
    }

  conf_dithering = aud_get_int(configSectionString, "dithering_mode");
  conf_latency_index = aud_get_int(configSectionString, "latency_index");
  conf_rev_zl = aud_get_int(configSectionString, "zero_latency");
  slotNumber = aud_get_int(configSectionString, "slotNumber");
  if(slotNumber <= 0) slotNumber = 1;
  
  fprintf(stderr, "Impulser2: init: %d slot(s)\n", slotNumber);
  
  // load Slot Config and autoload files
  slotVector->clear();
  currentSlotVector->clear();
  reverbVector->clear();
  for(int i = 1;i <= slotNumber;i ++)
    {
      SlotConfiguration slotC;
      slot_load(&slotC, i);
      if(store_inf(slotC.filename.c_str()) != 0) sprintf(inf, "(not loaded)");
      slotC.inf = inf;
      slotVector->push_back(slotC);
      set_rt_reverb(reverbVector->push_back(presetIRModelValue[conf_rev_zl]), &slotC);
    }
  StreamFs = 0;

  pthread_mutex_unlock(&plugin_mutex);
  return TRUE;
}

static void cleanup(void)
{
  fprintf(stderr, "Impulser2: cleanup()\n");
  if(UTILS::getSIMDFlag()&FV3_X86SIMD_FLAG_SSE) fprintf(stderr, "MXCSR: 0x%08x\n", UTILS::getMXCSR());

  pthread_mutex_lock(&plugin_mutex);
  plugin_available = false;

  if(conf_rev_dialog != NULL) gtk_widget_destroy(GTK_WIDGET(conf_rev_dialog));
  fprintf(stderr, "Impulser2: WARNING: cleanup during play is not supported!!\n");
  if(validModel == true)
    {
      fprintf(stderr, "Impulser2: cleanup: vector %d, cvector %d\n", (int)slotVector->size(), (int)currentSlotVector->size());
      reverbVector->clear();
      StreamFs = 0;
      delete slotVector;
      delete currentSlotVector;
      delete reverbVector;
      validModel = false;
    }
  
  if(gdither_on == FALSE) gdither_free(pdither);
  fprintf(stderr, "Impulser2: cleanup: done.\n");

  pthread_mutex_unlock(&plugin_mutex);

#ifndef __GNUC__
  plugin_fini();
#endif
}

static int validNumber = 0;
static void mod_samples_f(pfloat_t * iL, pfloat_t * iR, pfloat_t * oL, pfloat_t * oR, gint length, gint srate)
{
  if(length <= 0) return;
  if(validModel != true) fprintf(stderr, "Impulser2: !validModel\n");
  if(pthread_mutex_trylock(&plugin_mutex) == EBUSY) return;
  if(plugin_available != true)
    {
      pthread_mutex_unlock(&plugin_mutex);
      return;
    }

  if((int)reverbVector->size() != slotNumber)
    {
      fprintf(stderr, "Impulser2: mod_samples: Slot %d -> %d\n", (int)reverbVector->size(), slotNumber);
      if((int)reverbVector->size() < slotNumber) // increase slot
        {
          SlotConfiguration slotC;
          slot_init(&slotC);
          while(1)
            {
              if((int)reverbVector->size() == slotNumber) break;
              IRBASE * model = reverbVector->push_back(presetIRModelValue[conf_rev_zl]);
              if(reverbVector->size() <= slotVector->size())
                set_rt_reverb(model, &(*slotVector)[reverbVector->size()-1]);
              currentSlotVector->push_back(slotC);
            }
        }
      else // decrease slot
        {
          while(1)
            {
              if((int)reverbVector->size() == slotNumber) break;
              reverbVector->pop_back();
              currentSlotVector->pop_back();
            }
        }
    }
  
  // reset all if srate or latency have been changed
  if(StreamFs != srate||latencyIndex != conf_latency_index)
    {
      fprintf(stderr, "Impulser2: mod_samples: Fs %d -> %d IRM %d\n", StreamFs, srate, conf_rev_zl);
      StreamFs = srate;
      latencyIndex = conf_latency_index;
      SlotConfiguration slotC;
      slot_init(&slotC);
      currentSlotVector->clear();
      for(int i = 0;i < slotNumber;i ++)
        {
          currentSlotVector->push_back(slotC);
          reverbVector->assign(i, presetIRModelValue[conf_rev_zl]);
          set_rt_reverb(reverbVector->at(i), &(*slotVector)[i]);
        }
      fprintf(stderr, "Impulser2: mod_samples: vector %d, cvector %d\n", (int)slotVector->size(), (int)currentSlotVector->size());
    }
  
  for(int i = 0;i < (int)reverbVector->size();i ++)
    {
      if(reverbVector->size() > slotVector->size()) return;
      // changed stretch/limit reload
      if((*currentSlotVector)[i].stretch != (*slotVector)[i].stretch||
         (*currentSlotVector)[i].limit != (*slotVector)[i].limit||
         strcmp((*currentSlotVector)[i].filename.c_str(), (*slotVector)[i].filename.c_str()) != 0)
        {
          fprintf(stderr, "Impulser2: mod_samples: typeid=%s\n", typeid(*(*reverbVector)[i]).name());
          (*currentSlotVector)[i].stretch = (*slotVector)[i].stretch;
          (*currentSlotVector)[i].limit = (*slotVector)[i].limit;
          if(latencyIndex >= presetLatencyMax) conf_latency_index = latencyIndex = 0;
          if(typeid(*(*reverbVector)[i]) == typeid(IRMODEL2))
            {
              fprintf(stderr, "Impulser2: mod_samples: irmodel2 %ld\n", presetLatencyValue[latencyIndex]);
              dynamic_cast<IRMODEL2*>((*reverbVector)[i])->setFragmentSize(presetLatencyValue[latencyIndex]);
            }
          if(typeid(*(*reverbVector)[i]) == typeid(IRMODEL2ZL))
            {
              fprintf(stderr, "Impulser2: mod_samples: irmodel2zl %ld\n", presetLatencyValue[latencyIndex]);
              dynamic_cast<IRMODEL2ZL*>((*reverbVector)[i])->setFragmentSize(presetLatencyValue[latencyIndex]);
            }
          if(typeid(*(*reverbVector)[i]) == typeid(IRMODEL3)
#ifdef ENABLE_PTHREAD
             ||typeid(*(*reverbVector)[i]) == typeid(IRMODEL3P)
#endif
             )
            {
              fprintf(stderr, "Impulser2: mod_samples: irmodel3/p %ld %ld\n", presetLatencyValue1[latencyIndex], presetLatencyValue2[latencyIndex]);
              dynamic_cast<IRMODEL3*>((*reverbVector)[i])->setFragmentSize(presetLatencyValue1[latencyIndex], presetLatencyValue2[latencyIndex]);
            }
          CFILELOADER fileLoader;
          double l_stretch = std::pow(static_cast<double>(std::sqrt(2)), static_cast<double>((*slotVector)[i].stretch));
          int ret = fileLoader.load((*slotVector)[i].filename.c_str(), srate, l_stretch, (*slotVector)[i].limit, SRC_SINC_BEST_QUALITY);
	  
          if(ret == 0)
            {
              (*reverbVector)[i]->loadImpulse(fileLoader.out.L, fileLoader.out.R, fileLoader.out.getsize());
              (*currentSlotVector)[i].filename = (*slotVector)[i].filename;
              (*currentSlotVector)[i].valid = 1;
              fprintf(stderr, "Impulser2: mod_samples: Slot[%d] \"%s\"(%ld)\n", i, (*slotVector)[i].filename.c_str(), (*reverbVector)[i]->getImpulseSize());
            }
          else
            {
              fprintf(stderr, "Impulser2: mod_samples: Slot[%d] IR load fail! ret=%d ", i, ret);
              fprintf(stderr, "<%s>\n", fileLoader.errstr());
              (*currentSlotVector)[i].valid = 0;
            }
        }

      // delay
      long iDelay = (int)((float)StreamFs*(*slotVector)[i].idelay/1000.0f);
      if((*reverbVector)[i]->getInitialDelay() != iDelay)
        {
          fprintf(stderr, "Impulser2: mod_samples: InitialDelay[%d] %ld -> %ld\n", i, (*reverbVector)[i]->getInitialDelay(), iDelay);
          (*reverbVector)[i]->setInitialDelay(iDelay);
        }
      
      // slot
      if((*slotVector)[i].i1o2_index < presetSlotModeMax&&(*slotVector)[i].i1o2_index >= 0)
        {
          if((*currentSlotVector)[i].i1o2_index != presetSlotModeValue[(*slotVector)[i].i1o2_index])
            {
              (*currentSlotVector)[i].i1o2_index = presetSlotModeValue[(*slotVector)[i].i1o2_index];
              fprintf(stderr, "Impulser2: mod_samples: conf_i1o2[%d] -> %d\n", i, (*currentSlotVector)[i].i1o2_index);
            }
        }
    }
  
  validNumber = 0;
  for(int i = 0;i < (int)reverbVector->size();i ++)
    {
      if((*currentSlotVector)[i].valid == 1&&(int)slotVector->size() > i)
        {
          unsigned options = FV3_IR_DEFAULT;
          if((*currentSlotVector)[i].i1o2_index == 1) options |= FV3_IR_MONO2STEREO;
          if((*currentSlotVector)[i].i1o2_index == 3) options |= FV3_IR_SWAP_LR;
          if((*slotVector)[i].wet <= MIN_DB)          options |= FV3_IR_MUTE_WET;
          if((*slotVector)[0].dry <= MIN_DB||i > 0)   options |= FV3_IR_MUTE_DRY;
          if((*slotVector)[i].lpf <= 0.0&&(*slotVector)[i].hpf <= 0.0) options |= FV3_IR_SKIP_FILTER;
          if((int)reverbVector->size() > i)
            {
              if((*reverbVector)[i]->getImpulseSize() > 0)
                {
                  if(typeid(*(*reverbVector)[i]) == typeid(IRMODEL1))
                    options = FV3_IR_DEFAULT;
                  if(i == 0)
                    (*reverbVector)[i]->processreplace(iL,iR,oL,oR,length,options);
                  else
                    (*reverbVector)[i]->processreplace(iL,iR,oL,oR,length,options|FV3_IR_SKIP_INIT);
                  validNumber ++;
                }
            }
        }
    }  
  pthread_mutex_unlock(&plugin_mutex);
}

static SLOTP origLR, orig, reverb;
static void mod_samples_d(gfloat * LR, gint samples, gint srate)
{
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
  mod_samples_f(orig.L,orig.R,reverb.L,reverb.R,samples,srate);
  if(validNumber <= 0) return;
  if(next_dithering != conf_dithering||gdither_on == FALSE)
    {
      fprintf(stderr, "Impulser2: mod_samples: Dither [%d] -> [%d]\n", conf_dithering, next_dithering);
      if(gdither_on == FALSE) gdither_free(pdither);
      pdither = gdither_new(presetDitherValue[next_dithering], 2, GDitherFloat, 16);
      gdither_on = TRUE;
      conf_dithering = next_dithering;
    }
#ifdef PLUGDOUBLE
  gdither_run(pdither, 0, samples, reverb.L, LR);
  gdither_run(pdither, 1, samples, reverb.R, LR);
#else
  gdither_runf(pdither, 0, samples, reverb.L, LR);
  gdither_runf(pdither, 1, samples, reverb.R, LR);
#endif
}

static gint plugin_rate = 0, plugin_ch = 0;
static void impulser2_start(gint * channels, gint * rate)
{
  fprintf(stderr, "Impulser2: start: Ch %d Fs %d\n", *channels, *rate);
  plugin_rate = *rate;
  plugin_ch = *channels;
}
static void impulser2_process(gfloat ** data, gint * samples)
{
  if(plugin_rate <= 0||plugin_ch != 2) return;
  mod_samples_d(*data, *samples/plugin_ch, plugin_rate);
}
static void impulser2_flush()
{
  fprintf(stderr, "Impulser2: flush:\n");
}
static void impulser2_finish(gfloat ** data, gint * samples)
{
  fprintf(stderr, "Impulser2: finish\n");
  impulser2_process(data,samples);
}

extern "C" {
  LibXmmsPluginTable libXmmsPluginTable = {
    productString, init, cleanup, about, configure, impulser2_start, impulser2_process, impulser2_flush, impulser2_finish,
#ifdef AUDACIOUS36
    make_config_widget, about_text,
#endif
  };
}
