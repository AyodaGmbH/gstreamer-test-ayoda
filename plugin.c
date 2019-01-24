#include "sink.h"

#include <stdbool.h>

GST_DEBUG_CATEGORY_EXTERN (gst_ayoda_sink_debug);

static gboolean
ayodaplugin_init (GstPlugin * ayodaplugin)
{
    GST_DEBUG_CATEGORY_INIT (gst_ayoda_sink_debug, "ayodasink",
                             0, "Ayoda Sink");

    bool ret;

    ret =  gst_element_register (ayodaplugin, "ayodasink", GST_RANK_NONE,
                                 GST_TYPE_AYODA_SINK);
    if (ret == false) {
        return ret;
    }

    return true;
}

#define PACKAGE "ayoda"

GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        ayodaplugin,
        "Ayoda Plugin",
        ayodaplugin_init,
        "0.0.1",
        "LGPL",
        "apu",
        "http://ayoda.net/"
        )
