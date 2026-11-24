#define _GNU_SOURCE
#include <libusb.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct libusb_device { int marker; };
struct libusb_device_handle { struct libusb_device *device; };
static struct libusb_device device = {0x4c505553};
static struct libusb_device_handle handle = {&device};
static struct libusb_interface_descriptor alt;
static struct libusb_interface iface;
static struct libusb_config_descriptor config;
static struct libusb_endpoint_descriptor endpoints[2];
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t event_cond = PTHREAD_COND_INITIALIZER;
static int interrupted;
static FILE *log_file;

static void log_call(const char *fmt, ...)
{
    va_list ap;
    const char *path = getenv("LOGGERPRO_USB_ENUM_LOG");
    if (!path) return;
    if (!log_file) log_file = fopen(path, "a");
    if (!log_file) return;
    va_start(ap, fmt);
    vfprintf(log_file, fmt, ap);
    va_end(ap);
    fputc('\n', log_file);
    fflush(log_file);
}

static void init_descriptors(void)
{
    memset(endpoints, 0, sizeof(endpoints));
    endpoints[0].bLength=7; endpoints[0].bDescriptorType=LIBUSB_DT_ENDPOINT;
    endpoints[0].bEndpointAddress=0x01; endpoints[0].bmAttributes=LIBUSB_TRANSFER_TYPE_BULK;
    endpoints[0].wMaxPacketSize=64;
    endpoints[1].bLength=7; endpoints[1].bDescriptorType=LIBUSB_DT_ENDPOINT;
    endpoints[1].bEndpointAddress=0x81; endpoints[1].bmAttributes=LIBUSB_TRANSFER_TYPE_BULK;
    endpoints[1].wMaxPacketSize=64;
    memset(&alt, 0, sizeof(alt));
    alt.bLength=9; alt.bDescriptorType=LIBUSB_DT_INTERFACE; alt.bInterfaceNumber=0;
    alt.bAlternateSetting=0; alt.bNumEndpoints=2; alt.bInterfaceClass=0xff; alt.endpoint=endpoints;
    iface.altsetting=&alt; iface.num_altsetting=1;
    memset(&config, 0, sizeof(config));
    config.bLength=9; config.bDescriptorType=LIBUSB_DT_CONFIG; config.wTotalLength=9+9+14;
    config.bNumInterfaces=1; config.bConfigurationValue=1; config.bmAttributes=0x80;
    config.MaxPower=50; config.interface=&iface;
}

int LIBUSB_CALL libusb_init(libusb_context **ctx) { if (ctx) *ctx=NULL; init_descriptors(); log_call("init"); return 0; }
void LIBUSB_CALL libusb_exit(libusb_context *ctx) { (void)ctx; log_call("exit"); }
int LIBUSB_CALL libusb_hotplug_register_callback(libusb_context *ctx, int events, int flags, int vendor, int product, int dev_class, libusb_hotplug_callback_fn cb, void *user, libusb_hotplug_callback_handle *out)
{
    (void)ctx; (void)vendor; (void)product; (void)dev_class;
    log_call("hotplug events=%x flags=%x", events, flags);
    if (out) *out=1;
    if ((flags & LIBUSB_HOTPLUG_ENUMERATE) && (events & LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)) {
        log_call("arrived vid=08f7 pid=0003");
        cb(NULL, &device, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, user);
    }
    return 0;
}
void LIBUSB_CALL libusb_hotplug_deregister_callback(libusb_context *ctx, libusb_hotplug_callback_handle cb) { (void)ctx;(void)cb;log_call("deregister"); }
void LIBUSB_CALL libusb_interrupt_event_handler(libusb_context *ctx) { (void)ctx; pthread_mutex_lock(&event_mutex); interrupted=1; pthread_cond_broadcast(&event_cond); pthread_mutex_unlock(&event_mutex); log_call("interrupt-events"); }
int LIBUSB_CALL libusb_handle_events(libusb_context *ctx) { (void)ctx; pthread_mutex_lock(&event_mutex); while (!interrupted) pthread_cond_wait(&event_cond,&event_mutex); interrupted=0; pthread_mutex_unlock(&event_mutex); return 0; }
int LIBUSB_CALL libusb_get_device_descriptor(libusb_device *d, struct libusb_device_descriptor *x) { if(d!=&device||!x)return LIBUSB_ERROR_INVALID_PARAM; memset(x,0,sizeof(*x)); x->bLength=18;x->bDescriptorType=LIBUSB_DT_DEVICE;x->bcdUSB=0x0200;x->bMaxPacketSize0=64;x->idVendor=0x08f7;x->idProduct=0x0003;x->bcdDevice=0x0100;x->bNumConfigurations=1;log_call("descriptor");return 0; }
int LIBUSB_CALL libusb_open(libusb_device *d, libusb_device_handle **out) { if(d!=&device||!out)return LIBUSB_ERROR_NO_DEVICE;*out=&handle;log_call("open");return 0; }
void LIBUSB_CALL libusb_close(libusb_device_handle *h) {(void)h;log_call("close");}
libusb_device * LIBUSB_CALL libusb_get_device(libusb_device_handle *h) { return h==&handle?&device:NULL; }
uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device *d) { return d==&device?1:0; }
uint8_t LIBUSB_CALL libusb_get_port_number(libusb_device *d) { return d==&device?5:0; }
int LIBUSB_CALL libusb_get_active_config_descriptor(libusb_device *d, struct libusb_config_descriptor **out) { if(d!=&device||!out)return LIBUSB_ERROR_NO_DEVICE;*out=&config;log_call("active-config");return 0; }
void LIBUSB_CALL libusb_free_config_descriptor(struct libusb_config_descriptor *x) {(void)x;}
struct libusb_transfer * LIBUSB_CALL libusb_alloc_transfer(int n) {(void)n;return calloc(1,sizeof(struct libusb_transfer));}
void LIBUSB_CALL libusb_free_transfer(struct libusb_transfer *x) { free(x); }
int LIBUSB_CALL libusb_submit_transfer(struct libusb_transfer *x) { log_call("submit ep=%02x len=%d",x?x->endpoint:0,x?x->length:0);return LIBUSB_ERROR_NOT_SUPPORTED; }
int LIBUSB_CALL libusb_cancel_transfer(struct libusb_transfer *x) {(void)x;return LIBUSB_ERROR_NOT_FOUND;}
int LIBUSB_CALL libusb_clear_halt(libusb_device_handle *h,unsigned char ep) {(void)h;(void)ep;return 0;}
const char * LIBUSB_CALL libusb_strerror(int e) { (void)e; return "virtual libusb backend"; }
