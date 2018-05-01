/**
 *  Freeverb3 Jack effect plugin Framework
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

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <jack/jack.h>

#include "CArg.hpp"
#include "freeverb/slot.hpp"
#include "freeverb/fv3_ch_tool.hpp"
#include "libsamplerate2/samplerate2.h"
 
#ifdef PLUGDOUBLE
typedef fv3::slot_ SLOT;
typedef double pfloat_t;
#else
typedef fv3::slot_f SLOT;
typedef float pfloat_t;
#endif

#include "so.h"

extern LibXmmsPluginTable libXmmsPluginTable;
static LibXmmsPluginTable * ptable = NULL;
static char pProductString[128];
static void plugin_table_init(void)
{
  ptable = &libXmmsPluginTable;
  snprintf(pProductString, 127, "%s", ptable->name);
}

static void dsp_init(void){ if(ptable != NULL) ptable->init(); }
static void dsp_cleanup(void){ if(ptable != NULL) ptable->cleanup(); }
static void dsp_configure(void){ if(ptable != NULL) ptable->configure(); }
static void dsp_about(void){ if(ptable != NULL) ptable->about(); }
static void dsp_start(gint * channels, gint * rate){ if(ptable != NULL) ptable->start(channels, rate); }
static void dsp_process(gfloat ** data, gint * samples){ if(ptable != NULL) ptable->process(data,samples); }

CArg args;

jack_port_t *inputL, *inputR, *outputL, *outputR;
jack_client_t *client;

static void vprocess(float * inL, float * inR, float * outL, float * outR, int nframes)
{
  float * tmp = new float[nframes*2];
  fv3::mergeChannelsV(2, nframes, tmp, inL, inR);
  dsp_process(&tmp, &nframes);
  fv3::splitChannelsV(2, nframes, tmp, outL, outR);
  delete[] tmp;
}

gint fs = 0, ch = 2;
static int process(jack_nframes_t nframes, void *arg)
{
  jack_default_audio_sample_t *inL, *inR, *outL, *outR;
  inL = (jack_default_audio_sample_t*)jack_port_get_buffer(inputL, nframes);
  inR = (jack_default_audio_sample_t*)jack_port_get_buffer(inputR, nframes);
  outL = (jack_default_audio_sample_t*)jack_port_get_buffer(outputL, nframes);
  outR = (jack_default_audio_sample_t*)jack_port_get_buffer(outputR, nframes);

  
  if(fs != (gint)jack_get_sample_rate(client))
    {
      fs = (gint)jack_get_sample_rate(client);
      dsp_start(&fs, &ch);
    }
  vprocess(inL, inR, outL, outR, nframes);
  return 0;
}

void jack_shutdown(void *arg)
{
  exit(1);
}

void help(const char * cmd)
{
  std::fprintf(stderr,
	       "Usage: %s [options]\n"
	       "[[Options]]\n"
	       "-id JACK Unique ID (default/0=pid)\n"
	       "[[Example]] %s -id 1\n"
	       "\n",
	       cmd, cmd);
}

static void destroy_window(GtkWidget *widget, gpointer data){ gtk_main_quit(); }

static void g_configure(gpointer data,guint action,GtkWidget *widget)
{
  dsp_configure();
}

static void g_about(gpointer data,guint action,GtkWidget *widget)
{
  dsp_about();
}

static void g_quit(gpointer data,guint action,GtkWidget *widget)
{
  destroy_window(widget, data);
}

static GtkActionEntry entries[] = 
{
  /* name, stock id, label */
  { "MainMenuAction", NULL, "Main(_M)", },
  /* name, stock id, label, accelerator, tooltip */
  { "ConfigureAction", GTK_STOCK_PREFERENCES, "Configure(_C)", "<control>C", "Configure", G_CALLBACK(g_configure), },
  { "AboutAction", GTK_STOCK_ABOUT, "About(_A)", "<control>A", "About", G_CALLBACK(g_about), },
  { "QuitAction", GTK_STOCK_QUIT, "Quit(_Q)", "<control>Q", "Quit", G_CALLBACK(g_quit), },
};

static guint n_entries = G_N_ELEMENTS(entries);

static const gchar *ui_info =
  "<ui>"
  "    <menubar name='MenuBar'>"
  "        <menu name='MainMenu' action='MainMenuAction'>"
  "            <menuitem name='Configure' action='ConfigureAction' />"
  "            <menuitem name='About' action='AboutAction' />"
  "            <separator />"
  "            <menuitem name='Quit' action='QuitAction' />"
  "        </menu>"
  "    </menubar>"
  "</ui>";


static GtkUIManager *createMenuManager()
{
  GtkActionGroup  *action_group;
  GtkUIManager    *menu_manager;
  GError          *gErrors = NULL;
  action_group = gtk_action_group_new("PluginActions");
  menu_manager = gtk_ui_manager_new();
  gtk_action_group_add_actions(action_group, entries, n_entries, NULL);
  gtk_ui_manager_insert_action_group(menu_manager, action_group, 0);
  if (!gtk_ui_manager_add_ui_from_string(menu_manager, ui_info, -1, &gErrors))
    {
      g_print("building menus failed: %s\n", gErrors->message);
      g_error_free(gErrors);
    }
    return menu_manager;
}

GtkWidget *window, *menubar, *vbox;
GtkWidget *menu_bar;

void winmain(void)
{
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  GtkUIManager *menu_manager;
  menu_manager = createMenuManager();
  if(menu_manager == NULL) return;
  
  char window_name[1024];
  std::snprintf(window_name, 1024, "JACK - %s", pProductString);

  gtk_window_set_title(GTK_WINDOW(window), window_name);
  gtk_widget_set_size_request(GTK_WIDGET(window), 500, 100);
  g_signal_connect(window,"destroy", G_CALLBACK(destroy_window), NULL);

  gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(menu_manager));

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
  gtk_container_add(GTK_CONTAINER(window),vbox);
  gtk_widget_show(vbox);

  gtk_box_pack_start(GTK_BOX(vbox),gtk_ui_manager_get_widget(menu_manager, "/MenuBar"),FALSE,TRUE,0);
  gtk_widget_show(window);
  gtk_main();
}

static void make_directory(const gchar * path, mode_t mode)
{
  if(mkdir(path, mode) == 0) return;
  if(errno == EEXIST) return;
  g_printerr("Could not create directory (%s): %s", path, g_strerror(errno));
}

static void make_user_dir(void)
{
  gchar *dirname = g_build_filename(g_get_home_dir(), BMP_RCPATH, NULL);
  const mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  make_directory(dirname, mode755);
}

int main(int argc, char **argv)
{
  plugin_table_init();
  g_thread_init(NULL);
  if (!g_thread_supported())
    {
      g_printerr("Sorry, threads isn't supported on your platform.\n\n"
		 "If you're on a libc5 based linux system and installed Glib & GTK+ before you\n"
		 "installed LinuxThreads you need to recompile Glib & GTK+.\n");
      exit(EXIT_FAILURE);
    }
  gdk_threads_init();
  if (!gtk_init_check(&argc, &argv))
    {
      if (argc < 2) {
	/* GTK check failed, and no arguments passed to indicate that user is intending to only remote control a running session */
	g_printerr("Unable to open display, exiting.");
	exit(EXIT_FAILURE);
      }
      exit(EXIT_SUCCESS);
    }
  
  std::fprintf(stderr, "Freeverb3 JACK effect plugins Framework\n");
  std::fprintf(stderr, "<"PACKAGE"-"VERSION"> [%s]\n", pProductString);
  std::fprintf(stderr, "sizeof(pfloat_t) = %d\n", (int)sizeof(pfloat_t));

  help(basename(argv[0]));
  if(args.registerArg(argc, argv) != 0) exit(-1);
  
  make_user_dir();

  char *client_name, *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;
  
  int uid = args.getLong("-id");
  if(uid <= 0) uid = getpid();
  char unique_name[1024];
  std::snprintf(unique_name, 1024, "%s_%d", basename(argv[0]), uid);
  client_name = unique_name;
  client = jack_client_open(client_name, options, &status, server_name);
  if (client == NULL)
    {
      std::fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
      if (status & JackServerFailed) std::fprintf(stderr, "Unable to connect to JACK server\n");
      exit (-1);
    }
  if (status & JackServerStarted) std::fprintf(stderr, "JACK server started\n");
  if (status & JackNameNotUnique)
    {
      client_name = jack_get_client_name(client);
      std::fprintf(stderr, "unique name `%s' assigned\n", client_name);
    }

  dsp_init();  
  jack_set_process_callback(client, process, 0);
  jack_on_shutdown(client, jack_shutdown, 0);
  inputL = jack_port_register(client, "inputL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  inputR = jack_port_register(client, "inputR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  outputL = jack_port_register(client, "outputL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  outputR = jack_port_register(client, "outputR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if(inputL == NULL||inputR == NULL||outputL == NULL||outputR == NULL)
    {
      std::fprintf(stderr, "no more JACK ports available\n");
      exit(-1);
    }
  
  if(jack_activate(client))
    {
      std::fprintf(stderr, "cannot activate client\n");
      exit(-1);
    }
  // dirty hack for saving options
  gfloat hack[10]; gint nf = 5;
  dsp_process((gfloat**)&hack, &nf);

  winmain();
  jack_client_close(client);

  dsp_process((gfloat**)&hack, &nf);

  dsp_cleanup();
  exit(0);
}
