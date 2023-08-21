#include <gtk/gtk.h>

#include "diyafm_app.h"
#include "diyafm_win.h"
#include "diyafm_prefs.h"

/**
 schema_source = Gio.SettingsSchemaSource.new_from_directory(
    os.path.expanduser("~/schemas"),
    Gio.SettingsSchemaSource.get_default(),
    False,
)
schema = schema_source.lookup('com.companyname.appname', False)
settings = Gio.Settings.new_full(schema, None, None)
settings.set_boolean('mybool', True)
 */
struct _DiyafmPrefs
{
    GtkDialog parent;
};

typedef struct __DiyafmPrefsPrivate DiyafmPrefsPrivate;

struct __DiyafmPrefsPrivate
{
    GSettings *settings;
    GtkWidget *font;
    GtkWidget *transition;
};

G_DEFINE_TYPE_WITH_PRIVATE(DiyafmPrefs, diyafm_prefs, GTK_TYPE_DIALOG)

static void diyafm_prefs_init(DiyafmPrefs *prefs)
{
    DiyafmPrefsPrivate *priv;

    priv = diyafm_prefs_get_instance_private(prefs);
    gtk_widget_init_template(GTK_WIDGET(prefs));
    priv->settings = g_settings_new("app.iohub.dev.diyafm");

    g_settings_bind(priv->settings, "font",
                    priv->font, "font",
                    G_SETTINGS_BIND_DEFAULT);
    g_settings_bind(priv->settings, "transition",
                    priv->transition, "active-id",
                    G_SETTINGS_BIND_DEFAULT);
}

static void diyafm_prefs_dispose(GObject *object)
{
    DiyafmPrefsPrivate *priv;

    priv = diyafm_prefs_get_instance_private(DIYAFM_PREFS(object));
    g_clear_object(&priv->settings);

    G_OBJECT_CLASS(diyafm_prefs_parent_class)->dispose(object);
}

static void diyafm_prefs_class_init(DiyafmPrefsClass *class)
{
    G_OBJECT_CLASS(class)->dispose = diyafm_prefs_dispose;
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),"/app/iohub/dev/diyafm/resources/prefs.ui");
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmPrefs, font);
    gtk_widget_class_bind_template_child_private(GTK_WIDGET_CLASS(class), DiyafmPrefs, transition);
}

DiyafmPrefs * diyafm_prefs_new(DiyafmWindow *win)
{
    return g_object_new(DIYAFM_PREFS_TYPE, "transient-for", win, "use-header-bar", TRUE, NULL);
}