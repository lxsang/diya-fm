#include <gtk/gtk.h>

#include "diyafm_app.h"
#include "diyafm_win.h"

struct _DiyafmApp
{
    GtkApplication parent;
};

G_DEFINE_TYPE(DiyafmApp, diyafm_app, GTK_TYPE_APPLICATION);

static void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
    g_application_quit(G_APPLICATION(app));
}

static GActionEntry app_entries[] =
{
    {"quit", quit_activated, NULL, NULL, NULL}
};

static void diyafm_app_startup(GApplication *app)
{
    const gchar *quit_accels[2] = {"<Ctrl>Q", NULL};
    G_APPLICATION_CLASS(diyafm_app_parent_class)->startup(app);
    g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", quit_accels);
}

static void diyafm_app_init(DiyafmApp *app)
{
}

static void diyafm_app_activate(GApplication *app)
{
    DiyafmWindow *win;
    win = diyafm_window_new(DIYAFM_APP(app));
    gtk_window_present(GTK_WINDOW(win));
}

static void diyafm_app_open(GApplication *app, GFile **files, gint n_files, const gchar *hint)
{
    GList *windows;
    DiyafmWindow *win;
    int i;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows)
        win = DIYAFM_WINDOW(windows->data);
    else
        win = diyafm_window_new(DIYAFM_APP(app));

    for (i = 0; i < n_files; i++)
    {
        diyafm_window_open(win, files[i]);
    }

    gtk_window_present(GTK_WINDOW(win));
}

static void diyafm_app_class_init(DiyafmAppClass *class)
{
    G_APPLICATION_CLASS(class)->startup = diyafm_app_startup;
    G_APPLICATION_CLASS(class)->activate = diyafm_app_activate;
    G_APPLICATION_CLASS(class)->open = diyafm_app_open;
}

DiyafmApp *diyafm_app_new(void)
{
    return g_object_new(DIYAFM_APP_TYPE, "application-id", "app.iohub.dev.diyafm", "flags", G_APPLICATION_HANDLES_OPEN, NULL);
}