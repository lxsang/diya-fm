#ifndef __DIYAFM_NOTIF_MSG_VIEW_H__
#define __DIYAFM_NOTIF_MSG_VIEW_H__

#include <gtk/gtk.h>

#define DEFAULT_MSG_TO 4 // second

#define DIYAFM_NOTIF_MSG_TYPE (diyafm_notif_msg_get_type())
G_DECLARE_FINAL_TYPE(DiyafmNotifMsg, diyafm_notif_msg, DIYAFM, NOTIF_MSG, GtkListBoxRow)

DiyafmNotifMsg *diyafm_notif_msg_new(const gchar* msg, guint timeout_sec);

#endif