#include <gtk/gtk.h>

#include "diyafm_app.h"
#include "diyafm_win.h"
#include "diyafm_prefs.h"
#include "diyafm_file_view.h"
#include "diyafm_utils.h"

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
    GtkWidget *toolbar;
    GtkWidget *noti_revealer;
    GtkWidget *noti_box;
    GtkWidget *content_box;
    GtkWidget *spinner;

    DiyafmFileView *file_view_left;
    DiyafmFileView *file_view_right;

    guint loading_count;
};
G_DEFINE_TYPE_WITH_PRIVATE(DiyafmWindow, diyafm_window, GTK_TYPE_APPLICATION_WINDOW);

static void preferences_activated(GSimpleAction *action, GVariant *parameter, gpointer win)
{
    DiyafmPrefs *prefs;
    // GtkWindow *win;

    // win = gtk_application_get_active_window(GTK_APPLICATION(app));
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

static void diyafm_window_init(DiyafmWindow *win)
{
    DiyafmWindowPrivate *priv;
    GtkBuilder *builder;
    GMenuModel *menu;
    GAction *action;

    priv = diyafm_window_get_instance_private(win);
    priv->loading_count = 0;
    gtk_widget_init_template(GTK_WIDGET(win));
    priv->settings = g_settings_new("app.iohub.dev.diyafm");

    priv->file_view_left = diyafm_file_view_new();
    priv->file_view_right = diyafm_file_view_new();

    // pack the file view to the panel
    gtk_box_pack_start(GTK_BOX(priv->content_box), GTK_WIDGET(priv->file_view_left), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->content_box), GTK_WIDGET(priv->file_view_right), TRUE, TRUE, 0);

    //gtk_box_set_child_packing(GTK_BOX(priv->content_box),GTK_WIDGET(priv->file_view_left),TRUE,TRUE,0,GTK_PACK_START);
    //gtk_box_set_child_packing(GTK_BOX(priv->content_box),GTK_WIDGET(priv->file_view_right),TRUE,TRUE,0,GTK_PACK_START);

    g_object_set(priv->file_view_left, "dir", g_file_new_for_path("/home/dany"), NULL);
    g_object_set(priv->file_view_right, "dir", g_file_new_for_path("/home/dany"), NULL);

    builder = gtk_builder_new_from_resource("/app/iohub/dev/diyafm/resources/gears-menu.ui");
    menu = G_MENU_MODEL(gtk_builder_get_object(builder, "menu"));
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(priv->gears), menu);
    g_object_unref(builder);

    action = g_settings_create_action(priv->settings, "show-words");
    g_action_map_add_action(G_ACTION_MAP(win), action);
    g_object_unref(action);

    g_action_map_add_action_entries(G_ACTION_MAP(win), win_action_entries, G_N_ELEMENTS(win_action_entries), win);

    gtk_widget_add_events(GTK_WIDGET(win), GDK_BUTTON_PRESS_MASK);

    //load css
    GtkCssProvider  *style_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(style_provider, "/app/iohub/dev/diyafm/resources/main.css");
    //gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(win)), GTK_STYLE_PROVIDER(style_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(priv->noti_box)), GTK_STYLE_PROVIDER(style_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    
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
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, toolbar);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, noti_revealer);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, noti_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, content_box);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmWindow, spinner);
    // define custom signal
    //g_signal_new("diyafm-loading", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
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


void diyafm_notify(GtkWidget * widget, guint timeout, gchar* fstring,...)
{
    GtkWidget * view = gtk_widget_get_toplevel(widget);
    if (!gtk_widget_is_toplevel (GTK_WIDGET(view)))
    {
        return;
    }

    int dlen;
    va_list arguments;
    gchar *msg;

    va_start(arguments, fstring);
    dlen = vsnprintf(0, 0, fstring, arguments) + 1;
    va_end(arguments);
    if ((msg = (gchar *)g_malloc(dlen)) != 0)
    {
        va_start(arguments, fstring);
        vsnprintf(msg, dlen, fstring, arguments);
        va_end(arguments);
        DiyafmWindow *win = DIYAFM_WINDOW(view);
    
        DiyafmWindowPrivate *priv = diyafm_window_get_instance_private(win);
        g_object_set(priv->noti_revealer, "reveal-child", TRUE, NULL);

        DiyafmNotifMsg *notif = diyafm_notif_msg_new(msg, timeout);
        gtk_list_box_insert(GTK_LIST_BOX(priv->noti_box), GTK_WIDGET(notif), -1);

        g_free(msg);
    }

}

void diyafm_loading(GtkWidget * widget)
{
    GtkWidget * view = gtk_widget_get_toplevel(widget);
    if (!gtk_widget_is_toplevel (GTK_WIDGET(view)))
    {
        return;
    }
    DiyafmWindow *win = DIYAFM_WINDOW(view);
    
    DiyafmWindowPrivate *priv = diyafm_window_get_instance_private(win);
    priv->loading_count++;
    g_object_set(priv->spinner, "active",TRUE, NULL);
}

void diyafm_loaded(GtkWidget * widget)
{
    GtkWidget * view = gtk_widget_get_toplevel(widget);
    if (!gtk_widget_is_toplevel (GTK_WIDGET(view)))
    {
        return;
    }
    DiyafmWindow *win = DIYAFM_WINDOW(view);
    
    DiyafmWindowPrivate *priv = diyafm_window_get_instance_private(win);
    priv->loading_count--;
    if(priv->loading_count <= 0)
    {
        priv->loading_count = 0;
        g_object_set(priv->spinner, "active",FALSE, NULL);
    }
}