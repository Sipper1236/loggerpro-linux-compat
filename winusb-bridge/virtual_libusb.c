#include <libusb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct libusb_device { int marker; };
struct libusb_device_handle { struct libusb_device *device; int claimed; };
struct pending { struct libusb_transfer *transfer; int ready; struct pending *next; };

static struct libusb_device virtual_device = {1};
static struct pending *pending_transfers;
static pthread_mutex_t pending_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pending_cond = PTHREAD_COND_INITIALIZER;

static struct libusb_endpoint_descriptor endpoints[] = {
    {.bLength=7, .bDescriptorType=LIBUSB_DT_ENDPOINT, .bEndpointAddress=0x01,
     .bmAttributes=LIBUSB_TRANSFER_TYPE_BULK, .wMaxPacketSize=64},
    {.bLength=7, .bDescriptorType=LIBUSB_DT_ENDPOINT, .bEndpointAddress=0x81,
     .bmAttributes=LIBUSB_TRANSFER_TYPE_BULK, .wMaxPacketSize=64},
    {.bLength=7, .bDescriptorType=LIBUSB_DT_ENDPOINT, .bEndpointAddress=0x82,
     .bmAttributes=LIBUSB_TRANSFER_TYPE_INTERRUPT, .wMaxPacketSize=8, .bInterval=1},
};
static struct libusb_interface_descriptor alternate_settings[] = {
    {.bLength=9, .bDescriptorType=LIBUSB_DT_INTERFACE, .bInterfaceNumber=0,
     .bAlternateSetting=0, .bNumEndpoints=3, .bInterfaceClass=0xff,
     .endpoint=endpoints},
};
static struct libusb_interface interfaces[] = {
    {.altsetting=alternate_settings, .num_altsetting=1},
};
static struct libusb_config_descriptor active_config = {
    .bLength=9, .bDescriptorType=LIBUSB_DT_CONFIG, .wTotalLength=9+9+3*7,
    .bNumInterfaces=1, .bConfigurationValue=1, .bmAttributes=0x80,
    .MaxPower=50, .interface=interfaces,
};

int LIBUSB_CALL libusb_init(libusb_context **ctx)
{
    if (ctx) *ctx = NULL;
    return 0;
}

void LIBUSB_CALL libusb_exit(libusb_context *ctx) { (void)ctx; }

ssize_t LIBUSB_CALL libusb_get_device_list(libusb_context *ctx, libusb_device ***list)
{
    libusb_device **items;
    (void)ctx;
    items = calloc(2, sizeof(*items));
    if (!items) return LIBUSB_ERROR_NO_MEM;
    items[0] = &virtual_device;
    *list = items;
    return 1;
}

void LIBUSB_CALL libusb_free_device_list(libusb_device **list, int unref_devices)
{
    (void)unref_devices;
    free(list);
}

int LIBUSB_CALL libusb_get_device_descriptor(libusb_device *device,
        struct libusb_device_descriptor *desc)
{
    if (device != &virtual_device || !desc) return LIBUSB_ERROR_INVALID_PARAM;
    memset(desc, 0, sizeof(*desc));
    desc->bLength = sizeof(*desc);
    desc->bDescriptorType = LIBUSB_DT_DEVICE;
    desc->bcdUSB = 0x0200;
    desc->bMaxPacketSize0 = 64;
    desc->idVendor = 0x08f7;
    desc->idProduct = 0x0003; /* Vernier Go!Link */
    desc->bcdDevice = 0x0100;
    desc->iManufacturer = 1;
    desc->iProduct = 2;
    desc->iSerialNumber = 3;
    desc->bNumConfigurations = 1;
    return 0;
}

int LIBUSB_CALL libusb_open(libusb_device *device, libusb_device_handle **handle)
{
    if (device != &virtual_device || !handle) return LIBUSB_ERROR_NO_DEVICE;
    *handle = calloc(1, sizeof(**handle));
    if (!*handle) return LIBUSB_ERROR_NO_MEM;
    (*handle)->device = device;
    return 0;
}

int LIBUSB_CALL libusb_set_auto_detach_kernel_driver(libusb_device_handle *handle, int enable)
{
    (void)enable;
    return handle ? 0 : LIBUSB_ERROR_NO_DEVICE;
}

int LIBUSB_CALL libusb_claim_interface(libusb_device_handle *handle, int iface)
{
    if (!handle) return LIBUSB_ERROR_NO_DEVICE;
    if (iface != 0) return LIBUSB_ERROR_NOT_FOUND;
    handle->claimed = 1;
    return 0;
}

int LIBUSB_CALL libusb_release_interface(libusb_device_handle *handle, int iface)
{
    if (!handle || iface != 0) return LIBUSB_ERROR_NO_DEVICE;
    handle->claimed = 0;
    return 0;
}

void LIBUSB_CALL libusb_close(libusb_device_handle *handle) { free(handle); }
libusb_device * LIBUSB_CALL libusb_get_device(libusb_device_handle *handle)
{ return handle ? handle->device : NULL; }

int LIBUSB_CALL libusb_get_active_config_descriptor(libusb_device *device,
        struct libusb_config_descriptor **config)
{
    if (device != &virtual_device || !config) return LIBUSB_ERROR_NO_DEVICE;
    *config = &active_config;
    return 0;
}

void LIBUSB_CALL libusb_free_config_descriptor(struct libusb_config_descriptor *config)
{ (void)config; }

struct libusb_transfer * LIBUSB_CALL libusb_alloc_transfer(int iso_packets)
{
    if (iso_packets) return NULL;
    return calloc(1, sizeof(struct libusb_transfer));
}

void LIBUSB_CALL libusb_free_transfer(struct libusb_transfer *transfer) { free(transfer); }

static void fill_read(struct libusb_transfer *transfer)
{
    static const unsigned char response[] = {0xde, 0xad, 0xbe, 0xef};
    int n = transfer->length;
    if (n > (int)sizeof(response)) n = sizeof(response);
    if (n > 0) memcpy(transfer->buffer, response, n);
    transfer->actual_length = n;
}

int LIBUSB_CALL libusb_submit_transfer(struct libusb_transfer *transfer)
{
    struct pending *op = calloc(1, sizeof(*op));
    if (!op) return LIBUSB_ERROR_NO_MEM;
    op->transfer = transfer;
    if (transfer->endpoint == 0x82)
    {
        transfer->actual_length = 0;
        transfer->status = LIBUSB_TRANSFER_ERROR;
    }
    else
    {
        if (transfer->endpoint & LIBUSB_ENDPOINT_IN) fill_read(transfer);
        else transfer->actual_length = transfer->length;
        transfer->status = LIBUSB_TRANSFER_COMPLETED;
        op->ready = 1;
    }
    pthread_mutex_lock(&pending_mutex);
    op->next = pending_transfers;
    pending_transfers = op;
    pthread_cond_broadcast(&pending_cond);
    pthread_mutex_unlock(&pending_mutex);
    return 0;
}

int LIBUSB_CALL libusb_cancel_transfer(struct libusb_transfer *transfer)
{
    struct pending *op;
    int found = 0;
    pthread_mutex_lock(&pending_mutex);
    for (op = pending_transfers; op; op = op->next)
        if (op->transfer == transfer)
        {
            transfer->status = LIBUSB_TRANSFER_CANCELLED;
            transfer->actual_length = 0;
            op->ready = 1;
            found = 1;
            pthread_cond_broadcast(&pending_cond);
            break;
        }
    pthread_mutex_unlock(&pending_mutex);
    return found ? 0 : LIBUSB_ERROR_NOT_FOUND;
}

int LIBUSB_CALL libusb_handle_events(libusb_context *ctx)
{
    struct pending **cursor, *op;
    (void)ctx;
    pthread_mutex_lock(&pending_mutex);
    for (;;)
    {
        for (cursor = &pending_transfers; *cursor && !(*cursor)->ready; cursor = &(*cursor)->next) {}
        if (*cursor) break;
        pthread_cond_wait(&pending_cond, &pending_mutex);
    }
    op = *cursor;
    *cursor = op->next;
    pthread_mutex_unlock(&pending_mutex);
    op->transfer->callback(op->transfer);
    free(op);
    return 0;
}

int LIBUSB_CALL libusb_control_transfer(libusb_device_handle *handle,
        uint8_t request_type, uint8_t request, uint16_t value, uint16_t index,
        unsigned char *data, uint16_t length, unsigned int timeout)
{
    static const unsigned char device_desc[18] = {
        18, LIBUSB_DT_DEVICE, 0x00, 0x02, 0, 0, 0, 64,
        0xf7, 0x08, 0x03, 0x00, 0x00, 0x01, 1, 2, 3, 1
    };
    int n = length;
    (void)index; (void)timeout;
    if (!handle || request != LIBUSB_REQUEST_GET_DESCRIPTOR ||
        !(request_type & LIBUSB_ENDPOINT_IN) || (value >> 8) != LIBUSB_DT_DEVICE)
        return LIBUSB_ERROR_NOT_SUPPORTED;
    if (n > (int)sizeof(device_desc)) n = sizeof(device_desc);
    if (n) memcpy(data, device_desc, n);
    return n;
}
