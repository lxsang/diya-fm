#ifndef __DIYAFMWIN_H__
#define __DIYAFMWIN_H__

#include <gtk/gtk.h>
#include "diyafm_app.h"

#define DIYAFM_WINDOW_TYPE (diyafm_window_get_type())
G_DECLARE_FINAL_TYPE(DiyafmWindow, diyafm_window, DIYAFM, WINDOW, GtkApplicationWindow)

DiyafmWindow *diyafm_window_new(DiyafmApp *app);
void diyafm_window_open(DiyafmWindow *win,GFile *file);

#endif