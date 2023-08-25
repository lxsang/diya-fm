#include <gtk/gtk.h>

#include "diyafm_file_view.h"
#include "diyafm_utils.h"
#include "diyafm_file_entry.h"

#define MAX_FILE_RENDER_PER_TICK 128u
static void diyafm_refresh_view(DiyafmFileView *self);

struct _DiyafmFileView
{
    GtkLayout parent;
    GtkWidget *btn_search;
    GtkWidget *searchbar;
    GtkWidget *path_entry;
    GtkWidget *file_list;
    GtkWidget *btn_opts;
    GtkWidget *btn_edit;
    // properties
    gboolean show_hidden;
    gchar *sort;
    GFile *dir;

    GAsyncQueue *task_queue;
};

enum
{
    PROP_SHOW_HIDDEN = 1,
    PROP_DIR,
    PROP_SORT,
    NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE(DiyafmFileView, diyafm_file_view, GTK_TYPE_BOX)

static void file_view_show_info_activated(GSimpleAction *action, GVariant *parameter, gpointer object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    const char *action_name = g_action_get_name(G_ACTION(action));
    diyafm_notify(GTK_WIDGET(self), 3, "Execute action: %s", action_name);
}

static void file_view_sort_activated(GSimpleAction *action, GVariant *state, gpointer object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    const gchar *type = g_variant_get_string(state, NULL);
    if (g_str_equal(type, self->sort))
    {
        return;
    }
    g_action_change_state(G_ACTION(action), state);
    g_object_set(object, "sort", type, NULL);
}

static void file_view_refresh_activated(GSimpleAction *action, GVariant *parameter, gpointer object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    diyafm_refresh_view(self);
}

static GActionEntry file_view_opts_action_entries[] =
    {
        {"fileinfo", file_view_show_info_activated, NULL, NULL, NULL},
        {"newfile", file_view_show_info_activated, NULL, NULL, NULL},
        {"newfolder", file_view_show_info_activated, NULL, NULL, NULL},
        {"delete", file_view_show_info_activated, NULL, NULL, NULL},
        {"rename", file_view_show_info_activated, NULL, NULL, NULL},
        {"refresh", file_view_refresh_activated, NULL, NULL, NULL},
        {"sort", file_view_sort_activated, "s", "'type'", NULL},
        //{"show-hidden", quit_activated, NULL, NULL, NULL}
};

static void diyafm_file_view_init(DiyafmFileView *view)
{
    GtkBuilder *builder;
    GMenuModel *menu;

    gtk_widget_init_template(GTK_WIDGET(view));
    // priv->settings = g_settings_new("app.iohub.dev.diyafm");
    g_object_bind_property(view->btn_search, "active",
                           view->searchbar, "search-mode-enabled",
                           G_BINDING_BIDIRECTIONAL);
    view->task_queue = g_async_queue_new();

    builder = gtk_builder_new_from_resource("/app/iohub/dev/diyafm/resources/file-view-opts.ui");
    menu = G_MENU_MODEL(gtk_builder_get_object(builder, "menu"));
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(view->btn_opts), menu);
    g_object_unref(builder);

    builder = gtk_builder_new_from_resource("/app/iohub/dev/diyafm/resources/file-view-edit.ui");
    menu = G_MENU_MODEL(gtk_builder_get_object(builder, "menu"));
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(view->btn_edit), menu);
    g_object_unref(builder);

    GSimpleActionGroup *group = g_simple_action_group_new();
    g_action_map_add_action_entries(G_ACTION_MAP(group), file_view_opts_action_entries, G_N_ELEMENTS(file_view_opts_action_entries), view);

    GAction *action = (GAction *)g_property_action_new("show-hidden", view, "show-hidden");
    g_action_map_add_action(G_ACTION_MAP(group), action);
    g_object_unref(action);

    gtk_widget_insert_action_group(GTK_WIDGET(view), "fileview", G_ACTION_GROUP(group));
    // action = g_settings_create_action(priv->settings, "show-words");
    // g_action_map_add_action(G_ACTION_MAP(win), action);
    // g_object_unref(action);
}

static void diyafm_file_view_dispose(GObject *object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    if (self->dir && G_IS_OBJECT(self->dir))
    {
        g_object_unref(self->dir);
    }
    g_async_queue_unref(self->task_queue);
    G_OBJECT_CLASS(diyafm_file_view_parent_class)->dispose(object);
}

static void diyafm_remove_list_item_cb(GtkWidget *child, gpointer udata)
{
    gtk_container_remove(GTK_CONTAINER(udata), GTK_WIDGET(child));
}

static gboolean diyafm_idle_file_entry_rendering(gpointer object)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    gint len = g_async_queue_length(self->task_queue);
    if (len <= 0)
    {
        diyafm_loaded(GTK_WIDGET(self));
        return FALSE;
    }
    if (len > MAX_FILE_RENDER_PER_TICK)
    {
        len = MAX_FILE_RENDER_PER_TICK;
    }
    for (size_t i = 0; i < len; i++)
    {
        GFile *file = g_async_queue_pop(self->task_queue);
        if (file)
        {
            DiyafmFileEntry *entry = diyafm_file_entry_new(file);
            if (entry)
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
                g_async_queue_push(self->task_queue, file);
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
    if (len > 0)
    {
        for (size_t i = 0; i < len; i++)
        {
            GFile *file = g_async_queue_pop(self->task_queue);
            if (file)
            {
                g_object_unref(file);
            }
        }
        /**
         * the queue is not empty means that the loading action
         * is pending
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

    g_file_enumerate_children_async(self->dir,
                                    G_FILE_ATTRIBUTE_STANDARD_NAME,
                                    G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                    G_PRIORITY_LOW,
                                    NULL,
                                    diyafm_file_enum_cb,
                                    self);
}

static gint diyafm_file_view_sort_by_name_cb(GtkListBoxRow *row1, GtkListBoxRow *row2, gpointer user_data)
{
    gchar *name1, *name2;

    g_object_get(row1, "file-name", &name1, NULL);
    g_object_get(row2, "file-name", &name2, NULL);
    gchar *fold1 = g_utf8_casefold(name1, -1);
    gchar *fold2 = g_utf8_casefold(name2, -1);
    guint ret = g_strcmp0(fold1, fold2);
    g_free(name1);
    g_free(name2);
    g_free(fold1);
    g_free(fold2);
    return ret;
}

static gint diyafm_file_view_sort_by_size_cb(GtkListBoxRow *row1, GtkListBoxRow *row2, gpointer user_data)
{
    guint64 size1, size2;

    g_object_get(row1, "file-size", &size1, NULL);
    g_object_get(row2, "file-size", &size2, NULL);
    if (size1 < size2)
        return 1;
    if (size1 > size2)
        return -1;
    return 0;
}

static gint diyafm_file_view_sort_by_type_cb(GtkListBoxRow *row1, GtkListBoxRow *row2, gpointer user_data)
{
    guint type1, type2;

    g_object_get(row1, "file-type", &type1, NULL);
    g_object_get(row2, "file-type", &type2, NULL);
    if (type1 < type2)
        return -1;
    if (type1 > type2)
        return 1;
    return diyafm_file_view_sort_by_name_cb(row1, row2, user_data);
}

static void diyafm_file_view_sort(DiyafmFileView *self)
{
    if (g_strcmp0(self->sort, "type") == 0)
    {
        gtk_list_box_set_sort_func(GTK_LIST_BOX(self->file_list), diyafm_file_view_sort_by_type_cb, NULL, NULL);
        return;
    }
    if (g_strcmp0(self->sort, "name") == 0)
    {
        gtk_list_box_set_sort_func(GTK_LIST_BOX(self->file_list), diyafm_file_view_sort_by_name_cb, NULL, NULL);
        return;
    }
    if (g_strcmp0(self->sort, "size") == 0)
    {
        gtk_list_box_set_sort_func(GTK_LIST_BOX(self->file_list), diyafm_file_view_sort_by_size_cb, NULL, NULL);
        return;
    }
    diyafm_notify(GTK_WIDGET(self), DEFAULT_MSG_TO, "Unsupported sorting method: '%s'");
}

static gboolean diyafm_file_view_hidden_file_filter(GtkListBoxRow *row, gpointer user_data)
{
    gchar *name;
    g_object_get(row, "file-name", &name, NULL);
    gboolean ret = (name[0] == '.');
    g_free(name);
    return !ret;
}

static void diyafm_file_view_set_property(GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(obj);
    gchar *str;
    switch (property_id)
    {
    case PROP_SHOW_HIDDEN:
        self->show_hidden = g_value_get_boolean(value);
        if (!self->show_hidden)
        {
            gtk_list_box_set_filter_func(GTK_LIST_BOX(self->file_list), diyafm_file_view_hidden_file_filter, NULL, NULL);
        }
        else
        {
            gtk_list_box_set_filter_func(GTK_LIST_BOX(self->file_list), NULL, NULL, NULL);
        }
        break;
    case PROP_SORT:
        str = g_value_dup_string(value);
        if (g_strcmp0(self->sort, str) != 0)
        {
            if (self->sort)
            {
                g_free(self->sort);
            }
            self->sort = str;
            diyafm_file_view_sort(self);
        }
        else
        {
            g_free(str);
        }
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
    case PROP_SORT:
        g_value_set_string(value, self->sort);
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

static void diyafm_file_view_row_active_cb(gpointer object, GtkListBoxRow *row, GtkListBox *box)
{
    DiyafmFileView *self = DIYAFM_FILE_VIEW(object);
    GFile * file;
    g_object_get(row, "file", &file, NULL);
    
    GFileType type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);

    if (type == G_FILE_TYPE_DIRECTORY)
    {
        g_object_set(G_OBJECT(self), "dir", file, NULL);
    }
    else
    {
        char *path = g_file_get_path(file);
        // TODO error handle for the case G_FILE_TYPE_UNKNOWN, file does not exists
        diyafm_notify(GTK_WIDGET(self), DEFAULT_MSG_TO, "Open file: %s", path);
        if (path)
        {
            g_free(path);
        }
    }
    g_object_unref(file);
}

static void diyafm_file_view_class_init(DiyafmFileViewClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/app/iohub/dev/diyafm/resources/file-view.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, btn_search);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, searchbar);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, file_list);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, path_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, btn_opts);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmFileView, btn_edit);

    G_OBJECT_CLASS(class)->set_property = diyafm_file_view_set_property;
    G_OBJECT_CLASS(class)->get_property = diyafm_file_view_get_property;
    G_OBJECT_CLASS(class)->dispose = diyafm_file_view_dispose;

    properties[PROP_SHOW_HIDDEN] =
        g_param_spec_boolean("show-hidden",
                             "Show hidden file/folder",
                             "Whether the file view should show hidden files/folders",
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
    /**do not use G_PARAM_EXPLICIT_NOTIFY, otherwise, the menu show-hidden checkout wont updated*/
    properties[PROP_SORT] =
        g_param_spec_string("sort",
                            "Sort file",
                            "Sort file by type/size/name/date",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
    properties[PROP_DIR] =
        g_param_spec_object("dir",
                            "Current file object",
                            "File view shall show all files in this folder",
                            G_TYPE_OBJECT,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties(G_OBJECT_CLASS(class), NUM_PROPERTIES, properties);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), diyafm_file_view_path_entry_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), diyafm_file_view_btn_up_cb);
    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), diyafm_file_view_row_active_cb);
}

DiyafmFileView *diyafm_file_view_new()
{
    DiyafmFileView *view = g_object_new(
        DIYAFM_FILE_VIEW_TYPE,
        "show-hidden", FALSE,
        "dir", NULL,
        "sort", "type",
        NULL);

    return view;
}