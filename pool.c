#include "pool.h"

#include <gst/allocators/gstdmabuf.h>

#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/ioctl.h>

#define GET_DMA_FD        _IOWR('q', 13, struct al5_dma_info)
#define GET_DMA_PHY       _IOWR('q', 18, struct al5_dma_info)

#include <linux/types.h>

struct al5_dma_info {
    __u32 fd;
    __u32 size;
    __u32 phy_addr;
};

#define HORIZ_ALIGNMENT 16
#define VIDEO_FRAME_STRIDE(WIDTH) ((WIDTH) + (WIDTH) % HORIZ_ALIGNMENT)

G_DEFINE_TYPE(GstAyodaBufferPool, gst_ayoda_buffer_pool, GST_TYPE_VIDEO_BUFFER_POOL)

static GstFlowReturn gst_ayoda_buffer_pool_alloc_buffer(GstBufferPool * bpool, GstBuffer ** buffer, GstBufferPoolAcquireParams * params)
{
    GstAyodaBufferPool* pool = GST_AYODA_BUFFER_POOL (bpool);

    struct al5_dma_info info;

    info.size = RGBAUHDBUFFER_SIZE;

    int ret = ioctl(pool->vcu_fd, GET_DMA_FD, &info);
    if (ret < 0)
        return GST_FLOW_ERROR;

    int fd = info.fd;

    g_return_val_if_fail (fd >= 0, GST_FLOW_ERROR);

    GstMemory * mem = gst_dmabuf_allocator_alloc(pool->allocator, fd, RGBAUHDBUFFER_SIZE);

    g_return_val_if_fail (mem != NULL, GST_FLOW_ERROR);

    GstBuffer *buf = gst_buffer_new ();

    gst_buffer_append_memory (buf, mem);

    gsize offset[GST_VIDEO_MAX_PLANES] ;
    gint stride[GST_VIDEO_MAX_PLANES];

    if (GST_VIDEO_FORMAT_NV12 == pool->video_info.finfo->format) {
        assert(GST_VIDEO_INFO_N_PLANES (&pool->video_info) == 2);

        gint video_stride = VIDEO_FRAME_STRIDE(GST_VIDEO_INFO_WIDTH (&pool->video_info));
        gint row_length = video_stride * GST_VIDEO_INFO_HEIGHT (&pool->video_info);

        offset[0] = 0;
        offset[1] = video_stride * row_length;

        stride[0] = video_stride;
        stride[1] = video_stride;
    } else if (GST_VIDEO_FORMAT_RGBA == pool->video_info.finfo->format) {
        assert(GST_VIDEO_INFO_N_PLANES (&pool->video_info) == 1);

        gint video_stride = VIDEO_FRAME_STRIDE(4 * GST_VIDEO_INFO_WIDTH (&pool->video_info));

        offset[0] = 0;
        stride[0] = video_stride;
    } else {
        return GST_FLOW_ERROR;
    }

    gst_buffer_add_video_meta_full (buf, GST_VIDEO_FRAME_FLAG_NONE,
                GST_VIDEO_INFO_FORMAT (&pool->video_info),
                GST_VIDEO_INFO_WIDTH (&pool->video_info),
                GST_VIDEO_INFO_HEIGHT (&pool->video_info),
                GST_VIDEO_INFO_N_PLANES (&pool->video_info), offset, stride);

    *buffer = buf;
    return GST_FLOW_OK;
}

static gboolean gst_ayoda_buffer_pool_set_config (GstBufferPool * bpool, GstStructure * config)
{
  GstAyodaBufferPool* pool = GST_AYODA_BUFFER_POOL (bpool);
  GstCaps *caps;
  guint size, min;

  GST_OBJECT_LOCK (pool);

  if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min, NULL)) {
        GST_OBJECT_UNLOCK (pool);
        return FALSE;
  }

  if (caps == NULL) {
      GST_OBJECT_UNLOCK (pool);
      return FALSE;
    }

  GstVideoInfo info;

  if (!gst_video_info_from_caps (&info, caps)){
      GST_OBJECT_UNLOCK (pool);
      return FALSE;
    }

  pool->video_info = info;

  if (pool->caps)
    gst_caps_unref (pool->caps);
  pool->caps = gst_caps_ref (caps);

  gst_buffer_pool_config_set_allocator (config, pool->allocator, NULL);

  GST_OBJECT_UNLOCK (pool);

  return GST_BUFFER_POOL_CLASS (gst_ayoda_buffer_pool_parent_class)->set_config(bpool, config);
}

static void
gst_ayoda_buffer_pool_class_init (GstAyodaBufferPoolClass * klass)
{
  GstBufferPoolClass *gstbufferpool_class = (GstBufferPoolClass *) klass;

  gstbufferpool_class->alloc_buffer = gst_ayoda_buffer_pool_alloc_buffer;
  gstbufferpool_class->set_config = gst_ayoda_buffer_pool_set_config;
}

static void gst_ayoda_buffer_pool_init (GstAyodaBufferPool * pool)
{
    pool->allocator = gst_dmabuf_allocator_new();
    pool->vcu_fd = open("/dev/allegroDecodeIP", O_RDWR);
    assert(pool->vcu_fd >= 0);
}

GstBufferPool *gst_ayoda_buffer_pool_new ()
{
    GstAyodaBufferPool *pool = g_object_new(gst_ayoda_buffer_pool_get_type (), NULL);

    return GST_BUFFER_POOL_CAST(pool);
}



