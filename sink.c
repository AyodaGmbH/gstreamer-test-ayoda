#include "sink.h"

#include <gst/video/video.h>
#include <gst/allocators/gstdmabuf.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#define VIDEO_FRAME_ALIGNMENT 16
#define VIDEO_FRAME_STRIDE(WIDTH) ((WIDTH) + (WIDTH) % VIDEO_FRAME_ALIGNMENT)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS("video/x-raw"));

GST_DEBUG_CATEGORY (gst_ayoda_sink_debug);

enum
{
    PROP_0,
    PROP_USE_DMABUF,
};

#define DEFAULT_USE_DMABUF true

#define _do_init GST_DEBUG_CATEGORY_INIT (gst_ayoda_sink_debug, "ayodasink", 0, "ayodasink element");
#define gst_ayoda_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstAyodaSink, gst_ayoda_sink, GST_TYPE_BASE_SINK, _do_init)

static void gst_ayoda_sink_set_property (GObject * object, guint prop_id,
                                         const GValue * value, GParamSpec * pspec);
static void gst_ayoda_sink_get_property (GObject * object, guint prop_id,
                                         GValue * value, GParamSpec * pspec);
static void gst_ayoda_sink_finalize (GObject * obj);

static gboolean gst_ayoda_sink_set_caps(GstBaseSink * bsink, GstCaps * caps);

static gboolean gst_ayoda_sink_start (GstBaseSink * bsink);
static gboolean gst_ayoda_sink_stop (GstBaseSink * bsink);

static GstFlowReturn gst_ayoda_sink_render (GstBaseSink * bsink,
                                            GstBuffer * buffer);
static gboolean gst_ayoda_sink_unlock(GstBaseSink * basesink);
static gboolean gst_ayoda_sink_unlock_stop(GstBaseSink * basesink);

static gboolean gst_ayoda_sink_propose_allocation(GstBaseSink *sink, GstQuery *query);

static void
gst_ayoda_sink_class_init (GstAyodaSinkClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstBaseSinkClass *gstbase_sink_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gstelement_class = GST_ELEMENT_CLASS (klass);
    gstbase_sink_class = GST_BASE_SINK_CLASS (klass);

    gobject_class->set_property = gst_ayoda_sink_set_property;
    gobject_class->get_property = gst_ayoda_sink_get_property;
    gobject_class->finalize = gst_ayoda_sink_finalize;

    g_object_class_install_property (gobject_class, PROP_USE_DMABUF,
          g_param_spec_boolean ("use-dmabuf", "Use DMABuf",
              "Use DMABuf", DEFAULT_USE_DMABUF,
              G_PARAM_READWRITE | GST_PARAM_MUTABLE_READY |
              G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata (gstelement_class,
                                           "Ayoda Sink",
                                           "Sink",
                                           "Pushed data to RPU",
                                           "Victor Savu <vsavu@ayoda.net>");

    gst_element_class_add_static_pad_template (gstelement_class, &sinktemplate);

    gstbase_sink_class->set_caps = GST_DEBUG_FUNCPTR (gst_ayoda_sink_set_caps);
    gstbase_sink_class->render = GST_DEBUG_FUNCPTR (gst_ayoda_sink_render);
    gstbase_sink_class->start = GST_DEBUG_FUNCPTR (gst_ayoda_sink_start);
    gstbase_sink_class->stop = GST_DEBUG_FUNCPTR (gst_ayoda_sink_stop);
    gstbase_sink_class->unlock = GST_DEBUG_FUNCPTR (gst_ayoda_sink_unlock);
    gstbase_sink_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_ayoda_sink_unlock_stop);
    gstbase_sink_class->propose_allocation = GST_DEBUG_FUNCPTR (gst_ayoda_sink_propose_allocation);
}

static void
gst_ayoda_sink_init (GstAyodaSink * ayodasink)
{
    gst_base_sink_set_sync (GST_BASE_SINK (ayodasink), true);
    gst_base_sink_set_qos_enabled(GST_BASE_SINK (ayodasink), false);

    ayodasink->use_dmabuf = DEFAULT_USE_DMABUF;

    ayodasink->pool = GST_AYODA_BUFFER_POOL(gst_ayoda_buffer_pool_new());
}

static void
gst_ayoda_sink_finalize (GObject * obj)
{
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
gst_ayoda_sink_set_caps(GstBaseSink * bsink, GstCaps * caps)
{
    GstAyodaSink* sink = GST_AYODA_SINK(bsink);

    return TRUE;
}

static void
gst_ayoda_sink_set_property (GObject * object, guint prop_id,
                             const GValue * value, GParamSpec * pspec)
{
    GstAyodaSink* sink= GST_AYODA_SINK(object);

    switch (prop_id) {
    case PROP_USE_DMABUF:
        sink->use_dmabuf = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gst_ayoda_sink_get_property (GObject * object, guint prop_id, GValue * value,
                             GParamSpec * pspec)
{
    GstAyodaSink* sink= GST_AYODA_SINK(object);

    switch (prop_id) {
    case PROP_USE_DMABUF:
        g_value_set_boolean(value, sink->use_dmabuf);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static gboolean gst_ayoda_sink_start (GstBaseSink * bsink)
{
    GstAyodaSink *sink = GST_AYODA_SINK_CAST (bsink);

    return true;
}

static gboolean gst_ayoda_sink_stop (GstBaseSink * bsink)
{
    GstAyodaSink *sink = GST_AYODA_SINK_CAST (bsink);

    return true;
}

static GstFlowReturn gst_ayoda_sink_render (GstBaseSink * bsink, GstBuffer * buf)
{
    GstAyodaSink *sink = GST_AYODA_SINK_CAST (bsink);

    return GST_FLOW_OK;
}

static gboolean gst_ayoda_sink_unlock(GstBaseSink * basesink)
{
  GstAyodaSink *sink = GST_AYODA_SINK_CAST (basesink);

  GST_LOG_OBJECT (sink, "Unlocking");
  GST_OBJECT_LOCK (sink);
  sink->unlock = true;
  GST_OBJECT_UNLOCK (sink);

  return false;
}

static gboolean gst_ayoda_sink_unlock_stop(GstBaseSink * basesink)
{
  GstAyodaSink *sink = GST_AYODA_SINK_CAST (basesink);

  GST_LOG_OBJECT (sink, "Stopping unlock");
  GST_OBJECT_LOCK (sink);
  sink->unlock = false;
  GST_OBJECT_UNLOCK (sink);

  return true;
}

static gboolean gst_ayoda_sink_propose_allocation(GstBaseSink *basesink, GstQuery *query)
{
    GstAyodaSink *sink = GST_AYODA_SINK_CAST (basesink);

    if (sink->use_dmabuf) {
        gst_object_ref (sink->pool);
        gst_query_add_allocation_pool(query, GST_BUFFER_POOL_CAST(sink->pool), RGBAUHDBUFFER_SIZE, 5, 10);
    }

    return true;
}
