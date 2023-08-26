#ifndef __DIYAFM_UTILS_H__
#define __DIYAFM_UTILS_H__

#include <time.h>
#include "diyafm_notif_msg.h"

#define SIZE_KB 1024u
#define SIZE_MB (1024u*SIZE_KB)
#define SIZE_GB (1024u*SIZE_MB)

void timestr(time_t time, char* buf,int len,char* format, int gmt);
void diyafm_notify(GtkWidget * widget, guint timeout, gchar* msg,...);
void diyafm_loading(GtkWidget * widget);
void diyafm_loaded(GtkWidget * widget);

gchar* diyafm_get_file_size_text(guint64 size);
void diyafm_io_consume_lines(int fd,void (*cb)(char*,gpointer),gpointer user_data, GError **error);

guint g_async_queue_empty(GAsyncQueue* queue, GDestroyNotify notif);
//void diyafm_loading(GtkWidget * widget, guint timeout, gchar* msg,...);
#endif