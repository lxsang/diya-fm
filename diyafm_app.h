#ifndef __DIYAFM_APP_H__
#define __DIYAFM_APP_H__

#include <gtk/gtk.h>

#define DIYAFM_APP_TYPE (diyafm_app_get_type())
G_DECLARE_FINAL_TYPE(DiyafmApp, diyafm_app, DIYAFM, APP, GtkApplication)

DiyafmApp *diyafm_app_new(void);

#endif