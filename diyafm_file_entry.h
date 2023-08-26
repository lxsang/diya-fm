#ifndef __DIYAFM_FILE_ENTRY_H__
#define __DIYAFM_FILE_ENTRY_H__

#include <gtk/gtk.h>

enum {
    FILE_ENTRY_TYPE_DIR=0,
    FILE_ENTRY_TYPE_FILE
};

#define DIYAFM_FILE_ENTRY_TYPE (diyafm_file_entry_get_type())
G_DECLARE_FINAL_TYPE(DiyafmFileEntry, diyafm_file_entry, DIYAFM, FILE_ENTRY, GtkListBoxRow)

DiyafmFileEntry *diyafm_file_entry_new(GFile *file);

#endif