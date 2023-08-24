#ifndef __DIYAFM_UTILS_H__
#define __DIYAFM_UTILS_H__

#include <time.h>
#include "diyafm_notif_msg.h"
void timestr(time_t time, char* buf,int len,char* format, int gmt);
void diyafm_notify(GtkWidget * widget, guint timeout, gchar* msg,...);
void diyafm_loading(GtkWidget * widget);
void diyafm_loaded(GtkWidget * widget);
//void diyafm_loading(GtkWidget * widget, guint timeout, gchar* msg,...);
#endif