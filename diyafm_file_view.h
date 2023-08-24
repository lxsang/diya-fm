#ifndef __DIYAFM_FILE_VIEW_H__
#define __DIYAFM_FILE_VIEW_H__

#include <gtk/gtk.h>

#define DIYAFM_FILE_VIEW_TYPE (diyafm_file_view_get_type())
G_DECLARE_FINAL_TYPE(DiyafmFileView, diyafm_file_view, DIYAFM, FILE_VIEW, GtkBox)

DiyafmFileView *diyafm_file_view_new();
#endif