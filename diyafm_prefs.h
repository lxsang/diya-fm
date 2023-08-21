#ifndef __DIYAFM_PREFS_H__
#define __DIYAFM_PREFS_H__

#include <gtk/gtk.h>
#include "diyafm_win.h"

#define DIYAFM_PREFS_TYPE (diyafm_prefs_get_type())
G_DECLARE_FINAL_TYPE(DiyafmPrefs, diyafm_prefs, DIYAFM, PREFS, GtkDialog)

DiyafmPrefs *diyafm_prefs_new(DiyafmWindow *win);

#endif