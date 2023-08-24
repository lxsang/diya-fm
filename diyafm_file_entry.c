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
    GtkWidget *lbl_date;
    GtkWidget *lbl_size;

    // properties
    GFile* file;
    GtkWidget *file_view;
};

enum
{
    PROP_FILE=1,
    PROP_FILE_VIEW,
    NUM_PROPERTIES,
};

enum {
    TYPE_DIR=0,
    TYPE_FILE
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
    case PROP_FILE:
        if (self->file)
        {
            g_object_unref(self->file);
        }
        self->file = g_value_get_object(value);
        gchar *basename;
        basename = g_file_get_basename(self->file);
        gtk_label_set_text(GTK_LABEL(self->lbl_file), basename);
        g_free(basename);
        GError * error;
        GFileType type = g_file_query_file_type (self->file,G_FILE_QUERY_INFO_NONE,NULL);
        // change ICON
        if(type == G_FILE_TYPE_DIRECTORY)
        {
            gtk_image_set_from_icon_name(GTK_IMAGE(self->file_icon), "gtk-directory", GTK_ICON_SIZE_DIALOG);
        }
        // get size and modified date
        gtk_widget_set_visible (GTK_WIDGET(self->lbl_date),FALSE);
        gtk_widget_set_visible (GTK_WIDGET(self->lbl_size),FALSE);
        GFileInfo *info = g_file_query_info (self->file,"standard::*,time::*",G_FILE_QUERY_INFO_NONE,NULL,&error);
        if(info)
        {
            goffset size = g_file_info_get_size (info);
            snprintf(tmp, sizeof(tmp), "%"PRIu64" Kb", size);
            gtk_widget_set_visible (GTK_WIDGET(self->lbl_size),TRUE);
            gtk_label_set_text(GTK_LABEL(self->lbl_size), tmp);

            GDateTime *mtime = g_file_info_get_modification_date_time(info);
            if(mtime)
            {
                gint64 stamp = g_date_time_to_unix (mtime);
                timestr(stamp,tmp,sizeof(tmp),"Modified: %a, %d %b %Y %H:%M:%S GMT",1);
                gtk_label_set_text(GTK_LABEL(self->lbl_date), tmp);
                gtk_widget_set_visible (GTK_WIDGET(self->lbl_date),TRUE);
                //g_object_unref(mtime);
            }
            g_object_unref(info);
        }
        break;
    case PROP_FILE_VIEW:
        self->file_view = g_value_get_object(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void diyafm_file_entry_get_property(GObject *obj, guint property_id, GValue *value, GParamSpec *pspec)
{
    DiyafmFileEntry *self = DIYAFM_FILE_ENTRY(obj);
    gchar* path;
    switch (property_id)
    {
    case PROP_FILE:
        g_value_set_object(value, self->file);
        break;
    case PROP_FILE_VIEW:
        g_value_set_object(value, self->file_view);
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
    G_OBJECT_CLASS(diyafm_file_entry_parent_class)->dispose(obj);
}

static gboolean diyafm_file_entry_button_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    DiyafmFileEntry *self = DIYAFM_FILE_ENTRY(widget);
    if(event->type == GDK_DOUBLE_BUTTON_PRESS)
    {
        char * path = g_file_get_path(self->file);
        GFileType type = g_file_query_file_type (self->file,G_FILE_QUERY_INFO_NONE,NULL);

        if(type == G_FILE_TYPE_DIRECTORY)
        {
            g_object_set(G_OBJECT(self->file_view), "dir", self->file ,NULL);
        }
        else
        {
            // TODO error handle for the case G_FILE_TYPE_UNKNOWN, file does not exists
            diyafm_notify(widget, DEFAULT_MSG_TO, "Open file: %s", path);
        }
        if(path)
        {
            g_free(path);
        }
        return TRUE;
    }
    return FALSE;
}

static void diyafm_file_entry_class_init(DiyafmFileEntryClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/app/iohub/dev/diyafm/resources/file-entry.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, file_icon);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, lbl_file);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, lbl_date);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileEntry, lbl_size);
    
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), diyafm_file_entry_button_press_event_cb);

    G_OBJECT_CLASS(class)->dispose = diyafm_file_entry_dispose;
    G_OBJECT_CLASS(class)->set_property = diyafm_file_entry_set_property;
    G_OBJECT_CLASS(class)->get_property = diyafm_file_entry_get_property;

    properties[PROP_FILE] =
        g_param_spec_object( "file",
                            "GFile object",
                            "Get current GFile object",
                            G_TYPE_OBJECT,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    properties[PROP_FILE_VIEW] =
        g_param_spec_object( "file-view",
                            "Parent file view",
                            "File view associated to this entry",
                            DIYAFM_FILE_VIEW_TYPE,
                            G_PARAM_READWRITE |G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
    
    g_object_class_install_properties(G_OBJECT_CLASS(class), NUM_PROPERTIES, properties);
}

DiyafmFileEntry *diyafm_file_entry_new(GFile *file, GtkWidget* fileview)
{
    if(!file)
        return NULL;
        
    GFileType type = g_file_query_file_type (file,G_FILE_QUERY_INFO_NONE,NULL);
    // file does not exists
    if(type == G_FILE_TYPE_UNKNOWN)
    {
        g_object_unref(file);
        return NULL;
    }

    return g_object_new(DIYAFM_FILE_ENTRY_TYPE,
                "file", file,
                "file-view", G_OBJECT(fileview),
            NULL);
}