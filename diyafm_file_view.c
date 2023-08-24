#include <gtk/gtk.h>


#include "diyafm_file_view.h"
#include "diyafm_utils.h"
#include "diyafm_file_entry.h"

#define MAX_FILE_RENDER_PER_TICK 127u


struct _DiyafmFileView
{
    GtkLayout parent;
    GtkWidget *btn_search;
    GtkWidget *searchbar;
    GtkWidget *path_entry;
    GtkWidget *file_list;
    // properties
    gboolean show_hidden;
    GFile *dir;

    GAsyncQueue *task_queue;
};

enum
{
    PROP_SHOW_HIDDEN = 1,
    PROP_DIR,
    NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE(DiyafmFileView, diyafm_file_view, GTK_TYPE_BOX)

static void diyafm_file_view_init(DiyafmFileView *view)
{
    gtk_widget_init_template(GTK_WIDGET(view));
    // priv->settings = g_settings_new("app.iohub.dev.diyafm");
    g_object_bind_property(view->btn_search, "active",
                           view->searchbar, "search-mode-enabled",
                           G_BINDING_BIDIRECTIONAL);
    view->task_queue = g_async_queue_new ();
}

static void diyafm_file_view_dispose(GObject *object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    if (self->dir && G_IS_OBJECT(self->dir))
    {
        g_object_unref(self->dir);
    }
    g_async_queue_unref (self->task_queue);
    G_OBJECT_CLASS(diyafm_file_view_parent_class)->dispose(object);
}

static void diyafm_remove_list_item_cb(GtkWidget *child, gpointer udata)
{
    gtk_container_remove(GTK_CONTAINER(udata), GTK_WIDGET(child));
}

static gboolean  diyafm_idle_file_entry_rendering(gpointer object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    gint len = g_async_queue_length (self->task_queue);
    if(len == 0)
    {
        diyafm_loaded(GTK_WIDGET(self));
        return FALSE;
    }
    if(len > MAX_FILE_RENDER_PER_TICK)
    {
        len = MAX_FILE_RENDER_PER_TICK;
    }
    for (size_t i = 0; i < len; i++)
    {
        GFile* file = g_async_queue_try_pop (self->task_queue);
        if(file)
        {
            DiyafmFileEntry *entry = diyafm_file_entry_new(file, GTK_WIDGET(self));
            if(entry)
            {
                gtk_list_box_insert(GTK_LIST_BOX(self->file_list), GTK_WIDGET(entry), -1);
            }
            else
            {
                gchar *basename = basename = g_file_get_basename(file);
                diyafm_notify(GTK_WIDGET(self), DEFAULT_MSG_TO, "Invalid file: %s", basename);
                g_free(basename);
            }
        }
    }
    return TRUE;
}

static void diyafm_file_enum_cb(GObject *object, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    GFileInfo *info;
    gboolean ret;
    DiyafmFileView *self = DIYAFM_FILE_VIEW(user_data);
    GFileEnumerator *enumerator = g_file_enumerate_children_finish(self->dir, res, &error);
    if (enumerator)
    {
        while (TRUE)
        {
            ret = g_file_enumerator_iterate(enumerator, &info, NULL, NULL, &error);
            if (info)
            {
                const gchar *name = g_file_info_get_name(info);
                GFile *file = g_file_get_child(self->dir, name);
                g_async_queue_push (self->task_queue,file);
            }
            else
            {
                break;
            }
        }
        g_object_unref(enumerator);
        // add idle task for rendering listbox
        (void)g_idle_add(diyafm_idle_file_entry_rendering, self);
    }
    if (error && error->code)
    {
        gchar *path = g_file_get_path(self->dir);
        diyafm_notify(GTK_WIDGET(self), DEFAULT_MSG_TO, "Error reading directory: %s\n%s", path, error->message);
        g_free(path);
    }
}

static void diyafm_refresh_view(DiyafmFileView *self)
{
    if (!self->dir)
    {
        return;
    }
    gtk_container_forall(GTK_CONTAINER(self->file_list), diyafm_remove_list_item_cb, self->file_list);
    // remove the remaining queue
    guint len = g_async_queue_length(self->task_queue);
    if(len > 0)
    {
        for (size_t i = 0; i < len; i++)
        {
            GFile* file = g_async_queue_try_pop (self->task_queue);
            if(file)
            {
                g_object_unref(file);
            }
        }
        /**
         * the queue is not empty means that the loading action
         * is pendint
         * 
         */
        diyafm_loaded(GTK_WIDGET(self));
    }

    gchar *path = g_file_get_path(self->dir);
    GFileType type = g_file_query_file_type(self->dir, G_FILE_QUERY_INFO_NONE, NULL);
    // file does not exists
    if (type != G_FILE_TYPE_DIRECTORY)
    {
        diyafm_notify(GTK_WIDGET(self), DEFAULT_MSG_TO, "Invalid directory: %s", path);
        g_free(path);
        return;
    }
    diyafm_loading(GTK_WIDGET(self));
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(self->path_entry));
    gtk_entry_buffer_set_text(buffer, path, -1);
    g_free(path);

    g_file_enumerate_children_async (self->dir,
                                 G_FILE_ATTRIBUTE_STANDARD_NAME,
                                 G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                 G_PRIORITY_LOW,
                                 NULL,
                                 diyafm_file_enum_cb,
                                 self);
}

static void diyafm_file_view_set_property(GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(obj);
    switch (property_id)
    {
    case PROP_SHOW_HIDDEN:
        self->show_hidden = g_value_get_boolean(value);
        // TODO refresh file view
        break;
    case PROP_DIR:
        if (self->dir)
        {
            g_object_unref(self->dir);
        }
        self->dir = g_value_dup_object(value);

        diyafm_refresh_view(self);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void diyafm_file_view_get_property(GObject *obj, guint property_id, GValue *value, GParamSpec *pspec)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(obj);
    switch (property_id)
    {
    case PROP_SHOW_HIDDEN:
        g_value_set_boolean(value, self->show_hidden);
        break;
    case PROP_DIR:
        g_value_set_object(value, self->dir);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static gboolean diyafm_file_view_path_entry_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(widget);
    GtkEntryBuffer *buffer;
    gchar *path;
    GFileType type;
    GFile *dir;
    if (event->type == GDK_KEY_PRESS)
    {
        switch (event->keyval)
        {
        case GDK_KEY_Return:
            buffer = gtk_entry_get_buffer(GTK_ENTRY(self->path_entry));
            path = (gchar *)gtk_entry_buffer_get_text(buffer);
            dir = g_file_new_for_path(path);
            type = g_file_query_file_type(dir, G_FILE_QUERY_INFO_NONE, NULL);
            // file does not exists
            if (type != G_FILE_TYPE_DIRECTORY)
            {
                g_object_unref(dir);
                diyafm_notify(GTK_WIDGET(widget), DEFAULT_MSG_TO, "Invalid directory: %s", path);
                // reset the buffer
                path = g_file_get_path(self->dir);
                gtk_entry_buffer_set_text(buffer, path, -1);
                g_free(path);
            }
            else
            {
                g_object_set(self, "dir", dir, NULL);
            }
            break;
        }
    }

    return FALSE;
}

static void diyafm_file_view_btn_up_cb(GObject *object, GParamSpec *pspec)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    GFile *parent = g_file_get_parent(self->dir);
    if (parent)
    {
        g_object_set(self, "dir", parent, NULL);
    }
}

static void diyafm_file_view_class_init(DiyafmFileViewClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/app/iohub/dev/diyafm/resources/file-view.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, btn_search);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, searchbar);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, file_list);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, path_entry);

    G_OBJECT_CLASS(class)->set_property = diyafm_file_view_set_property;
    G_OBJECT_CLASS(class)->get_property = diyafm_file_view_get_property;
    G_OBJECT_CLASS(class)->dispose = diyafm_file_view_dispose;

    properties[PROP_SHOW_HIDDEN] =
        g_param_spec_boolean("show-hidden",
                             "Show hidden file/folder",
                             "Whether the file view should show hidden files/folders",
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    properties[PROP_DIR] =
        g_param_spec_object("dir",
                            "Current file object",
                            "File view shall show all files in this folder",
                            G_TYPE_OBJECT,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties(G_OBJECT_CLASS(class), NUM_PROPERTIES, properties);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), diyafm_file_view_path_entry_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), diyafm_file_view_btn_up_cb);
}

DiyafmFileView *diyafm_file_view_new()
{
    return g_object_new(DIYAFM_FILE_VIEW_TYPE, "show-hidden", FALSE, "dir", NULL, NULL);
}