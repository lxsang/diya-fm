#include <gtk/gtk.h>

#include "diyafm_utils.h"

struct _DiyafmNotifMsg
{
    GtkLayout parent;
    
    GtkWidget *btn_close;
    GtkWidget *lbl_message;
    gchar* msg;

    gboolean removed;
};

enum
{
    PROP_MSG = 1,
    NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];


G_DEFINE_TYPE(DiyafmNotifMsg, diyafm_notif_msg, GTK_TYPE_LIST_BOX_ROW)

static void diyafm_notif_msg_init(DiyafmNotifMsg *view)
{

    gtk_widget_init_template(GTK_WIDGET(view));
}

static void diyafm_notif_msg_set_property(GObject *obj, guint property_id, const GValue *value, GParamSpec *pspec)
{
    DiyafmNotifMsg *self = DIYAFM_NOTIF_MSG(obj);
    switch (property_id)
    {
    case PROP_MSG:
        if (self->msg)
        {
            g_free(self->msg);
        }
        self->msg = g_value_dup_string(value);
        gtk_label_set_text(GTK_LABEL(self->lbl_message), self->msg );
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void diyafm_notif_msg_get_property(GObject *obj, guint property_id, GValue *value, GParamSpec *pspec)
{
    DiyafmNotifMsg *self = DIYAFM_NOTIF_MSG(obj);
    switch (property_id)
    {
    case PROP_MSG:
        g_value_set_string(value, self->msg);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
        break;
    }
}

static void diyafm_notif_msg_dispose(GObject *object)
{
    DiyafmNotifMsg *self = DIYAFM_NOTIF_MSG(object);
    /*if (self->msg)
    {
        g_free(self->msg);
    }*/
    G_OBJECT_CLASS(diyafm_notif_msg_parent_class)->dispose(object);
}

static void close_notification(GObject *object, GParamSpec *pspec)
{
    DiyafmNotifMsg *self = DIYAFM_NOTIF_MSG(object);
    
    if(self->removed)
        return;
    
    if (gtk_widget_in_destruction(GTK_WIDGET(self)))
        return;

    self->removed = TRUE;
    GtkWidget *box = gtk_widget_get_parent (GTK_WIDGET(self));
    gtk_container_remove(GTK_CONTAINER(box),GTK_WIDGET(self) );
}

static void diyafm_notif_msg_class_init(DiyafmNotifMsgClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/app/iohub/dev/diyafm/resources/noti-msg.ui");
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmNotifMsg, btn_close);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DiyafmNotifMsg, lbl_message);

    gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(class), close_notification);

    G_OBJECT_CLASS(class)->dispose = diyafm_notif_msg_dispose;
    G_OBJECT_CLASS(class)->set_property = diyafm_notif_msg_set_property;
    G_OBJECT_CLASS(class)->get_property = diyafm_notif_msg_get_property;

    properties[PROP_MSG] =
        g_param_spec_string("msg",
                            "Notification message",
                            "The message that will be displayed to user",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
    g_object_class_install_properties(G_OBJECT_CLASS(class), NUM_PROPERTIES, properties);
}


static gboolean msg_auto_remove(gpointer data)
{
    close_notification(data, NULL);
    return FALSE;

}

DiyafmNotifMsg * diyafm_notif_msg_new(const gchar* msg, guint timeout_sec)
{
    DiyafmNotifMsg *view =  g_object_new(DIYAFM_NOTIF_MSG_TYPE, "msg", msg, NULL);
    if(timeout_sec > 0)
    {
        g_timeout_add_seconds(timeout_sec, msg_auto_remove, view);
    }
    view->removed = FALSE;
    return view;
}