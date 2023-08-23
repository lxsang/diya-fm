#include <gtk/gtk.h>

#include "diyafm_app.h"
#include "diyafm_win.h"
#include "diyafm_prefs.h"
#include "diyafm_file_view.h"

struct _DiyafmWindow
{
    GtkApplicationWindow parent;
};
// G_DEFINE_TYPE(DiyafmWindow, diyafm_window, GTK_TYPE_APPLICATION_WINDOW);

typedef struct _DiyafmWindowPrivate DiyafmWindowPrivate;

struct _DiyafmWindowPrivate
{
    GSettings *settings;
    GtkWidget *gears;
    GtkWidget *panel_left;
    GtkWidget *panel_right;
    GtkWidget *toolbar;
    DiyafmFileView * file_view_left;
    DiyafmFileView * file_view_right;
};
G_DEFINE_TYPE_WITH_PRIVATE(DiyafmWindow, diyafm_window, GTK_TYPE_APPLICATION_WINDOW);



static void preferences_activated(GSimpleAction *action, GVariant *parameter, gpointer win)
{
    DiyafmPrefs *prefs;
    //GtkWindow *win;

    //win = gtk_application_get_active_window(GTK_APPLICATION(app));
    prefs = diyafm_prefs_new(DIYAFM_WINDOW(win));
    gtk_window_present(GTK_WINDOW(prefs));
}

static void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer win)
{
    g_application_quit(G_APPLICATION(gtk_window_get_application(win)));
}

static GActionEntry win_action_entries[] =
{
    {"preferences", preferences_activated, NULL, NULL, NULL},
    {"quit", quit_activated, NULL, NULL, NULL}
};

static GtkWidget* diyafm_init_file_view(DiyafmWindow *win, DiyafmWindowPrivate *priv, GtkWidget* panel)
{
    GtkBuilder *builder;
    builder = gtk_builder_new_from_resource("/app/iohub/dev/diyafm/resources/file-view.ui");
    GtkWidget *view;
    view = GTK_WIDGET(gtk_builder_get_object(builder, "file_view_box"));
    gtk_box_pack_start(GTK_BOX(panel),view,TRUE,TRUE,0);
    gtk_widget_show(view);
    g_object_unref(builder);
    return view;
}

static void diyafm_window_init(DiyafmWindow *win)
{
    DiyafmWindowPrivate *priv;
    GtkBuilder *builder;
    GMenuModel *menu;
    GAction *action;

    priv = diyafm_window_get_instance_private(win);
    gtk_widget_init_template(GTK_WIDGET(win));
    priv->settings = g_settings_new("app.iohub.dev.diyafm");


    priv->file_view_left = diyafm_file_view_new();
    priv->file_view_right = diyafm_file_view_new();
    
    // pack the fileviews to the panels
    gtk_box_pack_start(GTK_BOX(priv->panel_left),priv->file_view_left,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(priv->panel_right),priv->file_view_right,view,TRUE,TRUE,0);

    builder = gtk_builder_new_from_resource("/app/iohub/dev/diyafm/resources/gears-menu.ui");
    menu = G_MENU_MODEL(gtk_builder_get_object(builder, "menu"));
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(priv->gears), menu);
    g_object_unref(builder);


    action = g_settings_create_action(priv->settings, "show-words");
    g_action_map_add_action(G_ACTION_MAP(win), action);
    g_object_unref(action);

    g_action_map_add_action_entries(G_ACTION_MAP(win), win_action_entries, G_N_ELEMENTS(win_action_entries), win);
}

static void diyafm_window_dispose(GObject *object)
{
    DiyafmWindow *win;
    DiyafmWindowPrivate *priv;

    win = DIYAFM_WINDOW(object);
    priv = diyafm_window_get_instance_private(win);

    g_clear_object(&priv->settings);

    G_OBJECT_CLASS(diyafm_window_parent_class)->dispose(object);
}

static void diyafm_window_class_init(DiyafmWindowClass *class)
{
    G_OBJECT_CLASS(class)->dispose = diyafm_window_dispose;
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/app/iohub/dev/diyafm/resources/diyafm.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, gears);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, panel_left);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, panel_right);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, toolbar);
}

DiyafmWindow *diyafm_window_new(DiyafmApp *app)
{
    return g_object_new(DIYAFM_WINDOW_TYPE, "application", app, NULL);
}

void diyafm_window_open(DiyafmWindow *win, GFile *file)
{
    DiyafmWindowPrivate *priv;
    gchar *basename;
    priv = diyafm_window_get_instance_private(win);
    basename = g_file_get_basename(file);
    g_free(basename);
}