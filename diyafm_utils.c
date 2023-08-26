#include "diyafm_utils.h"

void timestr(time_t time, char *buf, int len, char *format, int gmt)
{
    struct tm t;
    if (gmt)
    {
        gmtime_r(&time, &t);
    }
    else
    {
        localtime_r(&time, &t);
    }
    strftime(buf, len, format, &t);
}

gchar *diyafm_get_file_size_text(guint64 size)
{
    gdouble dsize;
    char *unit;
    if (size < SIZE_KB)
    {
        unit = "B";
        dsize = (gdouble)size;
    }
    else if (size < SIZE_MB)
    {
        unit = "kB";
        dsize = ((gdouble)size) / ((gdouble)SIZE_KB);
    }
    else if (size < SIZE_GB)
    {
        unit = "MB";
        dsize = ((gdouble)size) / ((gdouble)SIZE_MB);
    }
    else
    {
        unit = "GB";
        dsize = ((gdouble)size) / ((gdouble)SIZE_GB);
    }
    GString *s = g_string_new(NULL);
    g_string_append_printf(s, "%.1f %s", dsize, unit);
    return g_string_free(s, FALSE);
}

void diyafm_io_consume_lines(int fd,void (*cb)(char*,gpointer),gpointer user_data, GError **error)
{
    GIOChannel* channel = g_io_channel_unix_new (fd);
    GIOStatus status;
    gchar *str;
    while(TRUE)
    {
        
        status = g_io_channel_read_line (channel,&str,NULL,NULL,error);
        if(status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
        {
            break;
        }
        else if(status == G_IO_STATUS_NORMAL)
        {
            cb(str, user_data);
            g_free(str);
        }
    }
    g_io_channel_unref(channel);
}

guint g_async_queue_empty(GAsyncQueue* queue, GDestroyNotify notif)
{
    guint len = g_async_queue_length(queue);
    if (len > 0)
    {
        for (size_t i = 0; i < len; i++)
        {
            gpointer *pointer = g_async_queue_pop(queue);
            if (pointer)
            {
                notif(pointer);
            }
        }
    }
    return len;
}