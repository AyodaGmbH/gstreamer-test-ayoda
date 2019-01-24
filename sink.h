#ifndef __GST_AYODA_SINK_H__
#define __GST_AYODA_SINK_H__

#include <stdbool.h>

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <gst/video/video-color.h>

#include "pool.h"

G_BEGIN_DECLS

#define GST_TYPE_AYODA_SINK \
    (gst_ayoda_sink_get_type())
#define GST_AYODA_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AYODA_SINK,GstAyodaSink))
#define GST_AYODA_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AYODA_SINK,GstAyodaSinkClass))
#define GST_IS_AYODA_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AYODA_SINK))
#define GST_IS_AYODA_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AYODA_SINK))
#define GST_AYODA_SINK_CAST(obj) ((GstAyodaSink *)obj)

typedef struct _GstAyodaSink GstAyodaSink;
typedef struct _GstAyodaSinkClass GstAyodaSinkClass;

struct _GstAyodaSink {
    GstBaseSink element;

    bool use_dmabuf;
    bool unlock;

    GstAyodaBufferPool* pool;
};

struct _GstAyodaSinkClass {
    GstBaseSinkClass parent_class;
};

G_GNUC_INTERNAL GType gst_ayoda_sink_get_type (void);

G_END_DECLS

#endif /* __GST_AYODA_SINK_H__ */
