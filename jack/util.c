#ifdef HAVE_CONFIG_H
#include "fv3_config.h"
#endif

#include <string.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

static gboolean escape_cb(GtkWidget *widget, GdkEventKey *event, void (*action)(GtkWidget *widget))
{
  if (event->keyval == GDK_KEY_Escape)
    {
      action (widget);
      return TRUE;
    }
  return FALSE;
}

static void audgui_destroy_on_escape (GtkWidget * widget)
{
  g_signal_connect(widget, "key-press-event", (GCallback) escape_cb, gtk_widget_destroy);
}

void audgui_simple_message (GtkWidget **widget, GtkMessageType type, const gchar *title, const gchar *text)
{
  if(* widget != NULL)
    {
      const gchar * old = NULL;
      g_object_get ((GObject *) *widget, "text", &old, NULL);
      g_return_if_fail (old);
      gint messages = GPOINTER_TO_INT (g_object_get_data ((GObject *) *widget, "messages"));
      if (messages > 10) text = "\n(Further messages have been hidden.)";
      if (strstr (old, text)) goto CREATED;
      gchar both[strlen (old) + strlen (text) + 2];
      snprintf (both, sizeof both, "%s\n%s", old, text);
      g_object_set ((GObject *) *widget, "text", both, NULL);
      g_object_set_data ((GObject *) *widget, "messages", GINT_TO_POINTER(messages + 1));
      goto CREATED;
    }
  *widget = gtk_message_dialog_new (NULL, 0, type, GTK_BUTTONS_OK, "%s", text);
  gtk_window_set_title ((GtkWindow *) *widget, title);
  
  g_object_set_data ((GObject *) *widget, "messages", GINT_TO_POINTER (1));
  
  g_signal_connect (*widget, "response", (GCallback) gtk_widget_destroy, NULL);
  audgui_destroy_on_escape(*widget);
  g_signal_connect (*widget, "destroy", (GCallback) gtk_widget_destroyed, widget);
  
 CREATED:
  gtk_window_present ((GtkWindow *) *widget);
}
