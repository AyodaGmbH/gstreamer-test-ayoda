#ifndef __GST_AYODA_POOL_H__
#define __GST_AYODA_POOL_H__

#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#define RGBAUHDBUFFER_SIZE 33177600

G_BEGIN_DECLS

#define GST_TYPE_AYODA_BUFFER_POOL \
  (gst_ayoda_buffer_pool_get_type())
#define GST_AYODA_BUFFER_POOL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AYODA_BUFFER_POOL,GstAyodaBufferPool))
#define GST_IS_AYODA_BUFFER_POOL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AYODA_BUFFER_POOL))

typedef struct _GstAyodaBufferPool GstAyodaBufferPool;
typedef struct _GstAyodaBufferPoolClass GstAyodaBufferPoolClass;

struct _GstAyodaBufferPool
{
  GstVideoBufferPool parent;

  GstCaps *caps;
  GstVideoInfo video_info;

  GstAllocator *allocator;

  int vcu_fd;
};

struct _GstAyodaBufferPoolClass
{
  GstVideoBufferPoolClass parent_class;
};

GType gst_ayoda_buffer_pool_get_type(void);

GstBufferPool *gst_ayoda_buffer_pool_new();

G_END_DECLS

#endif
