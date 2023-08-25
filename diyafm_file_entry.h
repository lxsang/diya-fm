#ifndef __DIYAFM_FILE_ENTRY_H__
#define __DIYAFM_FILE_ENTRY_H__

#include <gtk/gtk.h>

#define DIYAFM_FILE_ENTRY_TYPE (diyafm_file_entry_get_type())
G_DECLARE_FINAL_TYPE(DiyafmFileEntry, diyafm_file_entry, DIYAFM, FILE_ENTRY, GtkListBoxRow)

DiyafmFileEntry *diyafm_file_entry_new(GFile *file);

#endif