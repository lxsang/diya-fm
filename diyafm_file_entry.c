#include <gtk/gtk.h>
#include <stdint.h>
#include <inttypes.h>

#include "diyafm_file_entry.h"
#include "diyafm_utils.h"
#include "diyafm_file_view.h"

struct _DiyafmFileEntry
{
    GtkLayout parent;

    GtkWidget *file_icon;
    GtkWidget *lbl_file;
    GtkWidget *lbl_detail;
    GtkWidget *lbl_size;

    // properties
    GFile *file;
    GtkWidget *file_view;
    guint type;
    guint64 size;
    gchar *name;
    gint64 stamp;
    gboolean search_mode;
};

enum
{
    PROP_FILE = 1,
    PROP_FILE_NAME,
    PROP_FILE_SIZE,
    PROP_FILE_TYPE,
    PROP_FILE_IS_HIDDEN,
    PROP_SEARCH_MODE,
    NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE(DiyafmFileEntry, diyafm_file_entry, GTK_TYPE_LIST_BOX_ROW)

static void diyafm_file_entry_init(DiyafmFileEntry *view)
{

    gtk_widget_init_template(GTK_WIDGET(view));
}

static void diyafm_file_entry_set_property(GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec)
{
    DiyafmFileEntry *self = DIYAFM_FILE_ENTRY(obj);
    gchar tmp[64];
    switch (property_id)
    {
    case PROP_SEARCH_MODE:
        self->search_mode = g_value_get_boolean(value);
        if(self->search_mode)
        {
            gchar* path = g_file_get_path(self->file);
            gtk_label_set_text(GTK_LABEL(self->lbl_detail), path);
            gtk_widget_set_visible(GTK_WIDGET(self->lbl_detail), TRUE);
            g_free(path);
        }
        else
        {
            timestr(self->stamp, tmp, sizeof(tmp), "Modified: %a, %d %b %Y %H:%M:%S GMT", 1);
            gtk_label_set_text(GTK_LABEL(self->lbl_detail), tmp);
            gtk_widget_set_visible(GTK_WIDGET(self->lbl_detail), TRUE);
        }
        break;
    case PROP_FILE:
        if (self->file)
        {
            g_object_unref(self->file);
        }
        if (self->name)
        {
            g_free(self->name);
        }
        self->file = g_value_get_object(value);
        self->name = g_file_get_basename(self->file);
        gtk_label_set_text(GTK_LABEL(self->lbl_file), self->name);
        GError *error;
        GFileType type = g_file_query_file_type(self->file, G_FILE_QUERY_INFO_NONE, NULL);
        // change ICON
        self->type = FILE_ENTRY_TYPE_FILE;
        if (type == G_FILE_TYPE_DIRECTORY)
        {
            //gtk_image_set_from_stock(GTK_IMAGE(self->file_icon), "gtk-directory", GTK_ICON_SIZE_DIALOG);
            gtk_image_set_from_icon_name(GTK_IMAGE(self->file_icon), "folder", GTK_ICON_SIZE_DIALOG);
            gtk_image_set_pixel_size(GTK_IMAGE(self->file_icon), 48);
            self->type = FILE_ENTRY_TYPE_DIR;
        }
        // get size and modified date
        gtk_widget_set_visible(GTK_WIDGET(self->lbl_detail), FALSE);
        gtk_widget_set_visible(GTK_WIDGET(self->lbl_size), FALSE);
        GFileInfo *info = g_file_query_info(self->file, "standard::*,time::*", G_FILE_QUERY_INFO_NONE, NULL, &error);
        if (info)
        {
            self->size = g_file_info_get_size(info);
            //if (self->type != FILE_ENTRY_TYPE_DIR)
            {
                gchar *size_s = diyafm_get_file_size_text(self->size);
                gtk_widget_set_visible(GTK_WIDGET(self->lbl_size), TRUE);
                gtk_label_set_text(GTK_LABEL(self->lbl_size), size_s);
                g_free(size_s);
            }

            GDateTime* mtime = g_file_info_get_modification_date_time(info);
            if (mtime)
            {
                self->stamp = g_date_time_to_unix(mtime);
                // g_object_unref(mtime);
            }
            g_object_unref(info);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void diyafm_file_entry_get_property(GObject *obj, guint property_id, GValue *value, GParamSpec *pspec)
{
    DiyafmFileEntry *self = DIYAFM_FILE_ENTRY(obj);
    gchar *path;
    switch (property_id)
    {
    case PROP_SEARCH_MODE:
        g_value_set_boolean(value, self->search_mode);
        break;
    case PROP_FILE:
        g_value_set_object(value, self->file);
        break;
    case PROP_FILE_SIZE:
        g_value_set_uint64(value, self->size);
        break;
    case PROP_FILE_TYPE:
        g_value_set_uint(value, self->type);
        break;
    case PROP_FILE_NAME:
        g_value_set_string(value, self->name);
        break;
    case PROP_FILE_IS_HIDDEN:
        g_value_set_boolean(value, self->name[0] == '.');
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void diyafm_file_entry_dispose(GObject *obj)
{
    DiyafmFileEntry *self = DIYAFM_FILE_ENTRY(obj);
    /*if (self->file && G_IS_OBJECT(self->file))
    {
        g_object_unref(self->file);
    }*/
    /*if(self->name)
    {
        g_free(self->name);
    }*/
    G_OBJECT_CLASS(diyafm_file_entry_parent_class)->dispose(obj);
}

/*
static gboolean diyafm_file_entry_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    DiyafmFileEntry *self = DIYAFM_FILE_ENTRY(widget);
    if(event->type == GDK_DOUBLE_BUTTON_PRESS)
    {
        return TRUE;
    }
    return FALSE;
}
*/

static void diyafm_file_entry_class_init(DiyafmFileEntryClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/app/iohub/dev/diyafm/resources/file-entry.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, file_icon);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, lbl_file);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, lbl_detail);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, lbl_size);

    G_OBJECT_CLASS(class)->dispose = diyafm_file_entry_dispose;
    G_OBJECT_CLASS(class)->set_property = diyafm_file_entry_set_property;
    G_OBJECT_CLASS(class)->get_property = diyafm_file_entry_get_property;

    properties[PROP_FILE] =
        g_param_spec_object("file",
                            "GFile object",
                            "Get current GFile object",
                            G_TYPE_OBJECT,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    properties[PROP_FILE_NAME] =
        g_param_spec_string("file-name",
                            "File name",
                            "Basename of current file",
                            NULL,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    properties[PROP_FILE_TYPE] =
        g_param_spec_uint("file-type",
                          "File type",
                          "File type: 0 for dir and 1 for file",
                          0, 1, 1,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
    
    properties[PROP_FILE_IS_HIDDEN] =
        g_param_spec_boolean("file-is-hidden",
                          "File is hidden",
                          "Whether file is hidden",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    properties[PROP_SEARCH_MODE] =
        g_param_spec_boolean("search-mode",
                             "Search mode",
                             "Activate/deactivate search mode",
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
    
    properties[PROP_FILE_SIZE] =
        g_param_spec_uint64("file-size",
                            "File size",
                            "File size in bytes",
                            0, UINT64_MAX, 0,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties(G_OBJECT_CLASS(class), NUM_PROPERTIES, properties);
}

DiyafmFileEntry *diyafm_file_entry_new(GFile *file)
{
    if (!file)
        return NULL;

    GFileType type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
    // file does not exists
    if (type == G_FILE_TYPE_UNKNOWN)
    {
        g_object_unref(file);
        return NULL;
    }

    return g_object_new(DIYAFM_FILE_ENTRY_TYPE,
                        "file", file,
                        "search-mode", FALSE,
                        NULL);
}