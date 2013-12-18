#ifndef XMMS_UTIL_H
#define XMMS_UTIL_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
void audgui_simple_message (GtkWidget **widget, GtkMessageType type, const gchar *title, const gchar *text);
G_END_DECLS

#endif
