#ifndef BUTTON_UI_H_
#define BUTTON_UI_H_

#include <gtk/gtk.h>

/* There are functions/vars in button_sim.c meant to be accessed only from the UI
 * side of the simulator.
 */

extern int time_to_quit;
extern gint key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);
extern gint key_release_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);

#endif
