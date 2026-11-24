#include <arpa/inet.h>
#include <errno.h>
#include <libusb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 47821
#define MAX_TRANSFER (1024 * 1024)
#define MAX_SESSIONS 64
struct session { uint32_t id; libusb_device_handle *usb; int iface; unsigned active; int closing; };
struct pending_transfer { uint32_t session_id; uint8_t endpoint; struct session *session; struct libusb_transfer *transfer; int done; enum libusb_transfer_status status; int actual; struct pending_transfer *next; };
static struct session sessions[MAX_SESSIONS];
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t session_cond = PTHREAD_COND_INITIALIZER;
static uint32_t next_id = 1;
static pthread_mutex_t transfer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t transfer_cond = PTHREAD_COND_INITIALIZER;
static struct pending_transfer *pending_transfers;
static int event_loop_running = 1;

static int read_exact(int fd, void *buffer, size_t length)
{
    unsigned char *p = buffer;
    while (length) { ssize_t n = recv(fd, p, length, 0); if (n <= 0) return 0; p += n; length -= n; }
    return 1;
}
static int write_exact(int fd, const void *buffer, size_t length)
{
    const unsigned char *p = buffer;
    while (length) { ssize_t n = send(fd, p, length, MSG_NOSIGNAL); if (n <= 0) return 0; p += n; length -= n; }
    return 1;
}
static uint32_t get32(const unsigned char *p) { return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void put32(unsigned char *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

static unsigned char encode_error(int error)
{
    switch (error)
    {
        case 0: return 0;
        case LIBUSB_ERROR_NO_DEVICE: return 3;
        case LIBUSB_ERROR_ACCESS: return 4;
        case LIBUSB_ERROR_TIMEOUT: return 5;
        case LIBUSB_ERROR_INVALID_PARAM: return 6;
        case LIBUSB_ERROR_BUSY: return 7;
        case LIBUSB_ERROR_NO_MEM: return 8;
        case LIBUSB_ERROR_NOT_SUPPORTED: return 9;
        case LIBUSB_ERROR_NOT_FOUND: return 10;
        default: return 1;
    }
}

struct supported_device_id { uint16_t vid, pid; };
static const struct supported_device_id supported_device_ids[] = {
    {0x08f7, 0x0001}, {0x08f7, 0x0002}, {0x08f7, 0x0003},
    {0x08f7, 0x0004}, {0x08f7, 0x0005}, {0x08f7, 0x0006},
    {0x08f7, 0x0007}, {0x08f7, 0x0008}, {0x08f7, 0x0009},
    {0x08f7, 0x000a}, {0x08f7, 0x000b}, {0x08f7, 0x000d},
    {0x08f7, 0x000e}, {0x08f7, 0x000f}, {0x08f7, 0x0011},
    {0x08f7, 0x0015}, {0x08f7, 0x0016}, {0x08f7, 0x0018},
    {0x2457, 0x1002}, {0x2457, 0x1012}, {0x2457, 0x1014},
    {0x2457, 0x1022}, {0x2457, 0x1024}, {0x2457, 0x1036},
};

static int is_supported_device(uint16_t vid, uint16_t pid)
{
    size_t i;
    for (i = 0; i < sizeof(supported_device_ids) / sizeof(supported_device_ids[0]); ++i)
        if (supported_device_ids[i].vid == vid && supported_device_ids[i].pid == pid) return 1;
    return 0;
}

static int open_supported_usb(uint16_t vid, uint16_t pid, int iface, libusb_device_handle **out)
{
    libusb_device **list = NULL;
    ssize_t count = libusb_get_device_list(NULL, &list), i;
    int ret = LIBUSB_ERROR_NO_DEVICE;
    if (count < 0) return (int)count;
    for (i = 0; i < count; ++i)
    {
        struct libusb_device_descriptor desc;
        libusb_device_handle *handle = NULL;
        if (libusb_get_device_descriptor(list[i], &desc) < 0) continue;
        if ((vid && desc.idVendor != vid) || (pid && desc.idProduct != pid)) continue;
        if (!is_supported_device(desc.idVendor, desc.idProduct)) continue;
        if (libusb_open(list[i], &handle) < 0) continue;
        libusb_set_auto_detach_kernel_driver(handle, 1);
        ret = libusb_claim_interface(handle, iface);
        if (ret == 0) { *out = handle; break; }
        libusb_close(handle);
    }
    libusb_free_device_list(list, 1);
    return ret;
}

static struct session *session_acquire(uint32_t id)
{
    unsigned i;
    struct session *s = NULL;
    pthread_mutex_lock(&session_mutex);
    for (i = 0; i < MAX_SESSIONS; ++i)
        if (sessions[i].id == id && !sessions[i].closing) { s = &sessions[i]; ++s->active; break; }
    pthread_mutex_unlock(&session_mutex);
    return s;
}
static void session_release(struct session *s)
{
    pthread_mutex_lock(&session_mutex);
    if (--s->active == 0) pthread_cond_broadcast(&session_cond);
    pthread_mutex_unlock(&session_mutex);
}
static struct session *session_create(uint16_t vid, uint16_t pid, int iface, int *status)
{
    libusb_device_handle *handle = NULL;
    unsigned i;
    *status = open_supported_usb(vid, pid, iface, &handle);
    if (*status) return NULL;
    pthread_mutex_lock(&session_mutex);
    for (i = 0; i < MAX_SESSIONS; ++i) if (!sessions[i].id) break;
    if (i == MAX_SESSIONS) { pthread_mutex_unlock(&session_mutex); libusb_release_interface(handle, iface); libusb_close(handle); *status = LIBUSB_ERROR_NO_MEM; return NULL; }
    sessions[i].id = next_id++; if (!next_id) next_id = 1;
    sessions[i].usb = handle; sessions[i].iface = iface; sessions[i].active = 0; sessions[i].closing = 0;
    pthread_mutex_unlock(&session_mutex);
    return &sessions[i];
}
static int session_destroy(uint32_t id)
{
    unsigned i;
    struct session *s = NULL;
    pthread_mutex_lock(&session_mutex);
    for (i = 0; i < MAX_SESSIONS; ++i)
        if (sessions[i].id == id) { s = &sessions[i]; break; }
    if (!s) { pthread_mutex_unlock(&session_mutex); return 11; }
    if (s->closing) { pthread_mutex_unlock(&session_mutex); return 11; }
    s->closing = 1;
    pthread_mutex_lock(&transfer_mutex);
    {
        struct pending_transfer *op;
        for (op = pending_transfers; op; op = op->next)
            if (op->session_id == id && op->transfer) libusb_cancel_transfer(op->transfer);
    }
    pthread_mutex_unlock(&transfer_mutex);
    while (s->active) pthread_cond_wait(&session_cond, &session_mutex);
    sessions[i].id = 0;
    pthread_mutex_unlock(&session_mutex);
    libusb_release_interface(s->usb, s->iface); libusb_close(s->usb);
    return 0;
}

static int endpoint_type(libusb_device_handle *handle, int iface, uint8_t endpoint)
{
    struct libusb_device *dev = libusb_get_device(handle);
    struct libusb_config_descriptor *config = NULL;
    int result = LIBUSB_ERROR_NOT_FOUND;
    /* The WinUSB imports used by Logger Pro do not change alternate settings. */
    int active_alt = 0;
    int i, j, k;
    int ret;
    ret = libusb_get_active_config_descriptor(dev, &config);
    if (ret < 0) return ret;
    for (i = 0; i < config->bNumInterfaces; ++i)
        for (j = 0; j < config->interface[i].num_altsetting; ++j)
        {
            const struct libusb_interface_descriptor *id = &config->interface[i].altsetting[j];
            if (id->bInterfaceNumber != iface || id->bAlternateSetting != active_alt) continue;
            for (k = 0; k < id->bNumEndpoints; ++k)
                if (id->endpoint[k].bEndpointAddress == endpoint)
                    result = id->endpoint[k].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;
        }
    libusb_free_config_descriptor(config);
    return result;
}
static void LIBUSB_CALL transfer_callback(struct libusb_transfer *transfer)
{
    struct pending_transfer *op = transfer->user_data, **cursor;
    pthread_mutex_lock(&transfer_mutex);
    op->status = transfer->status;
    op->actual = transfer->actual_length;
    op->done = 1;
    for (cursor = &pending_transfers; *cursor; cursor = &(*cursor)->next)
        if (*cursor == op) { *cursor = op->next; break; }
    pthread_cond_broadcast(&transfer_cond);
    pthread_mutex_unlock(&transfer_mutex);
    libusb_free_transfer(transfer);
}

static int transfer_pipe(struct session *s, uint32_t session_id, uint8_t endpoint, unsigned char *buffer,
        int length, int *actual, int *cancelled)
{
    struct pending_transfer *op;
    struct libusb_transfer *transfer;
    int type = endpoint_type(s->usb, s->iface, endpoint), ret;
    *actual = 0; *cancelled = 0;
    if (type < 0) return type;
    if (!(op = calloc(1, sizeof(*op)))) return LIBUSB_ERROR_NO_MEM;
    if (!(transfer = libusb_alloc_transfer(0))) { free(op); return LIBUSB_ERROR_NO_MEM; }
    op->session_id = session_id; op->endpoint = endpoint; op->session = s; op->transfer = transfer;
    if (type == LIBUSB_TRANSFER_TYPE_BULK)
        libusb_fill_bulk_transfer(transfer, s->usb, endpoint, buffer, length, transfer_callback, op, 10000);
    else if (type == LIBUSB_TRANSFER_TYPE_INTERRUPT)
        libusb_fill_interrupt_transfer(transfer, s->usb, endpoint, buffer, length, transfer_callback, op, 10000);
    else { libusb_free_transfer(transfer); free(op); return LIBUSB_ERROR_NOT_SUPPORTED; }

    pthread_mutex_lock(&transfer_mutex);
    op->next = pending_transfers; pending_transfers = op;
    ret = libusb_submit_transfer(transfer);
    if (ret < 0)
    {
        pending_transfers = op->next;
        pthread_mutex_unlock(&transfer_mutex);
        libusb_free_transfer(transfer); free(op); return ret;
    }
    while (!op->done) pthread_cond_wait(&transfer_cond, &transfer_mutex);
    ret = op->status == LIBUSB_TRANSFER_COMPLETED ? 0 : LIBUSB_ERROR_IO;
    *actual = op->actual;
    if (op->status == LIBUSB_TRANSFER_CANCELLED) { *cancelled = 1; ret = LIBUSB_ERROR_INTERRUPTED; }
    else if (op->status == LIBUSB_TRANSFER_TIMED_OUT) ret = LIBUSB_ERROR_TIMEOUT;
    else if (op->status == LIBUSB_TRANSFER_NO_DEVICE) ret = LIBUSB_ERROR_NO_DEVICE;
    pthread_mutex_unlock(&transfer_mutex);
    free(op);
    return ret;
}

static void cancel_session_pipe(uint32_t id, uint8_t endpoint)
{
    struct pending_transfer *op;
    pthread_mutex_lock(&transfer_mutex);
    for (op = pending_transfers; op; op = op->next)
        if (op->session_id == id && op->endpoint == endpoint && op->transfer)
            libusb_cancel_transfer(op->transfer);
    pthread_mutex_unlock(&transfer_mutex);
}

static void *usb_event_thread(void *unused)
{
    (void)unused;
    while (event_loop_running) libusb_handle_events(NULL);
    return NULL;
}

static void *client_thread(void *opaque)
{
    int fd = *(int *)opaque;
    unsigned char op, hdr[12], reply[5] = {0};
    struct session *s = NULL;
    uint32_t id, length;
    int status = 0, actual = 0;
    free(opaque);
    if (!read_exact(fd, &op, 1)) goto done;
    if (op == 'O')
    {
        struct session *created;
        uint16_t vid, pid; int iface;
        if (!read_exact(fd, hdr, 5)) goto done;
        vid = hdr[0] | ((uint16_t)hdr[1] << 8); pid = hdr[2] | ((uint16_t)hdr[3] << 8); iface = hdr[4];
        created = session_create(vid, pid, iface, &status);
        reply[0] = encode_error(status);
        if (!status) put32(reply + 1, created->id);
        write_exact(fd, reply, 5);
        fprintf(stderr, "open usb vid=%04x pid=%04x iface=%u => %d session=%u\n", vid, pid, iface, status, created ? created->id : 0);
        goto done;
    }
    if (op == 'C')
    {
        if (!read_exact(fd, hdr, 4)) goto done;
        reply[0] = session_destroy(get32(hdr)); write_exact(fd, reply, 1); goto done;
    }
    if (op == 'A')
    {
        if (!read_exact(fd, hdr, 5)) goto done;
        id = get32(hdr); s = session_acquire(id);
        if (!s) reply[0] = 11;
        else { cancel_session_pipe(id, hdr[4]); session_release(s); s = NULL; }
        write_exact(fd, reply, 1); goto done;
    }
    if (op == 'W' || op == 'R')
    {
        if (!read_exact(fd, hdr, 9)) goto done;
        id = get32(hdr); length = get32(hdr + 5);
        if (length > MAX_TRANSFER) { reply[0] = 6; write_exact(fd, reply, 5); goto done; }
        unsigned char *data = malloc(length ? length : 1);
        if (!data) { reply[0] = 8; write_exact(fd, reply, 5); goto done; }
        if (op == 'W' && length && !read_exact(fd, data, length)) { free(data); goto done; }
        s = session_acquire(id);
        int cancelled = 0;
        status = s ? transfer_pipe(s, id, hdr[4], data, length, &actual, &cancelled) : LIBUSB_ERROR_NO_DEVICE;
        if (s) { session_release(s); s = NULL; }
        reply[0] = cancelled ? 2 : encode_error(status); put32(reply + 1, actual);
        if (!write_exact(fd, reply, 5) || (op == 'R' && actual && !write_exact(fd, data, actual))) { free(data); goto done; }
        fprintf(stderr, "%c session=%u endpoint=%02x length=%u actual=%d status=%d\n", op, id, hdr[4], length, actual, status);
        free(data); goto done;
    }
    if (op == 'D')
    {
        uint8_t type, index; uint16_t lang;
        if (!read_exact(fd, hdr, 12)) goto done;
        id = get32(hdr); type = hdr[4]; index = hdr[5]; lang = hdr[6] | ((uint16_t)hdr[7] << 8); length = get32(hdr + 8);
        if (length > MAX_TRANSFER) { reply[0] = 6; write_exact(fd, reply, 5); goto done; }
        unsigned char *data = malloc(length ? length : 1);
        if (!data) { reply[0] = 8; write_exact(fd, reply, 5); goto done; }
        s = session_acquire(id);
        status = s ? libusb_control_transfer(s->usb, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
            LIBUSB_REQUEST_GET_DESCRIPTOR, (type << 8) | index, lang, data, length, 10000) : LIBUSB_ERROR_NO_DEVICE;
        if (s) { session_release(s); s = NULL; }
        actual = status < 0 ? 0 : status;
        reply[0] = status < 0 ? encode_error(status) : 0; put32(reply + 1, actual);
        if (!write_exact(fd, reply, 5) || (actual && !write_exact(fd, data, actual))) { free(data); goto done; }
        free(data); goto done;
    }
done:
    if (s) session_release(s);
    close(fd); return NULL;
}

int main(void)
{
    int server, opt = 1;
    pthread_t event_thread;
    struct sockaddr_in addr;
    if (libusb_init(NULL) < 0) return 2;
    if (pthread_create(&event_thread, NULL, usb_event_thread, NULL)) return 2;
    pthread_detach(event_thread);
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) return 3;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr)); addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); addr.sin_port = htons(PORT);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(server, 32) < 0) { perror("bind/listen"); return 4; }
    fprintf(stderr, "Logger Pro USB bridge listening on 127.0.0.1:%d\n", PORT);
    for (;;)
    {
        int *client = malloc(sizeof(*client)); pthread_t thread;
        if (!client) break;
        *client = accept(server, NULL, NULL);
        if (*client < 0) { free(client); continue; }
        if (!pthread_create(&thread, NULL, client_thread, client)) pthread_detach(thread);
        else { close(*client); free(client); }
    }
    close(server); libusb_exit(NULL); return 0;
}
