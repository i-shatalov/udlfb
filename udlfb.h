#ifndef UDLFB_H
#define UDLFB_H

/*
 * TODO: Propose standard fb.h ioctl for reporting damage,
 * using _IOWR() and one of the existing area structs from fb.h
 * Consider these ioctls deprecated, but they're still used by the
 * DisplayLink X server as yet - need both to be modified in tandem
 * when new ioctl(s) are ready.
 */
#define DLFB_IOCTL_RETURN_EDID	 0xAD
#define DLFB_IOCTL_REPORT_DAMAGE 0xAA
struct dloarea {
	int x, y;
	int w, h;
	int x2, y2;
};

struct urb_node {
	struct list_head entry;
	struct dlfb_data *dev;
	struct delayed_work release_urb_work;
	struct urb *urb;
};

struct urb_list {
	struct list_head list;
	spinlock_t lock;
	struct semaphore limit_sem;
	int available;
	int count;
	size_t size;
};

struct dlfb_data {
	struct usb_device *udev;
	struct device *gdev; /* &udev->dev */
	struct fb_info *info;
	struct urb_list urbs;
	struct kref kref;
	char *backing_buffer;
	int fb_count;
	bool virtualized; /* true when physical usb device not present */
	struct delayed_work init_framebuffer_work;
	struct delayed_work free_framebuffer_work;
	atomic_t usb_active; /* 0 = update virtual buffer, but no usb traffic */
	atomic_t lost_pixels; /* 1 = a render op failed. Need screen refresh */
	char *edid; /* null until we read edid from hw or get from sysfs */
	size_t edid_size;
	int sku_pixel_limit;
	int base16;
	int base8;
	u32 pseudo_palette[256];
	int blank_mode; /*one of FB_BLANK_ */
	/* blit-only rendering path metrics, exposed through sysfs */
	atomic_t bytes_rendered; /* raw pixel-bytes driver asked to render */
	atomic_t bytes_identical; /* saved effort with backbuffer comparison */
	atomic_t bytes_sent; /* to usb, after compression including overhead */
	atomic_t cpu_kcycles_used; /* transpired during pixel processing */
};

#define NR_USB_REQUEST_I2C_SUB_IO 0x02
#define NR_USB_REQUEST_CHANNEL 0x12

/* -BULK_SIZE as per usb-skeleton. Can we get full page and avoid overhead? */
#define BULK_SIZE 512
#define MAX_TRANSFER (PAGE_SIZE*16 - BULK_SIZE)
#define WRITES_IN_FLIGHT (4)

#define MAX_VENDOR_DESCRIPTOR_SIZE 256

#define GET_URB_TIMEOUT	HZ
#define FREE_URB_TIMEOUT (HZ*2)

#define BPP                     2
#define MAX_CMD_PIXELS		255

#define RLX_HEADER_BYTES	7
#define MIN_RLX_PIX_BYTES       4
#define MIN_RLX_CMD_BYTES	(RLX_HEADER_BYTES + MIN_RLX_PIX_BYTES)

#define RLE_HEADER_BYTES	6
#define MIN_RLE_PIX_BYTES	3
#define MIN_RLE_CMD_BYTES	(RLE_HEADER_BYTES + MIN_RLE_PIX_BYTES)

#define RAW_HEADER_BYTES	6
#define MIN_RAW_PIX_BYTES	2
#define MIN_RAW_CMD_BYTES	(RAW_HEADER_BYTES + MIN_RAW_PIX_BYTES)

#define DL_DEFIO_WRITE_DELAY    5 /* fb_deferred_io.delay in jiffies */
#define DL_DEFIO_WRITE_DISABLE  (HZ*60) /* "disable" with long delay */

/* remove these once align.h patch is taken into kernel */
#define DL_ALIGN_UP(x, a) ALIGN(x, a)
#define DL_ALIGN_DOWN(x, a) ALIGN(x-(a-1), a)

/* for full kernel builds, pulled from drivers/video/edid.h */
#ifndef EDID_LENGTH
#define EDID_LENGTH 128
#endif

/* remove once this gets added to sysfs.h */
#define __ATTR_RW(attr) __ATTR(attr, 0644, attr##_show, attr##_store)

/*
 * udlfb is both a usb device, and a framebuffer device.
 * They may exist at the same time, but during various stages
 * inactivity, teardown, or "virtual" operation, only one or the
 * other will exist (one will outlive the other).  So we can't
 * call the dev_*() macros, because we don't have a stable dev object.
 */

#ifndef pr_err
#define pr_err(format, arg...)		\
	pr_err("udlfb: " format, ## arg)
#endif

#ifndef pr_warn
#define pr_warn(format, arg...) \
	pr_warning("udlfb: " format, ## arg)
#endif

#ifndef pr_notice
#define pr_notice(format, arg...) \
	pr_notice("udlfb: " format, ## arg)
#endif

#ifndef pr_info
#define pr_info(format, arg...) \
	pr_info("udlfb: " format, ## arg)
#endif

/* Let people on older kernels build udlfb as a module */
#ifndef vzalloc
#define vzalloc vmalloc
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 33)
#define usb_alloc_coherent usb_buffer_alloc
#define usb_free_coherent usb_buffer_free
#endif

#endif
