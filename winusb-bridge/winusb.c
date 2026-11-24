typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int ULONG;
typedef unsigned int DWORD;
typedef int BOOL;
typedef void *HANDLE;
typedef unsigned short WCHAR;
typedef unsigned char *PUCHAR;
typedef unsigned int *PULONG;
typedef unsigned int SOCKET;
typedef HANDLE WINUSB_INTERFACE_HANDLE;
typedef struct { volatile ULONG Internal, InternalHigh; ULONG Offset, OffsetHigh; HANDLE hEvent; } OVERLAPPED;
typedef OVERLAPPED *LPOVERLAPPED;
#define WINAPI __attribute__((stdcall))
#define TRUE 1
#define NULL ((void *)0)
#define FALSE 0
#define INVALID_SOCKET 0xffffffffu
#define ERROR_INVALID_HANDLE 6
#define ERROR_GEN_FAILURE 31
#define ERROR_DEVICE_NOT_CONNECTED 1167
#define ERROR_OPERATION_ABORTED 995
#define ERROR_INVALID_PARAMETER 87
#define ERROR_ACCESS_DENIED 5
#define ERROR_SEM_TIMEOUT 121
#define ERROR_BUSY 170
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_NOT_SUPPORTED 50
#define ERROR_NOT_FOUND 1168

extern int WINAPI WSAStartup(USHORT, void *) __asm__("WSAStartup@8");
extern SOCKET WINAPI socket(int, int, int) __asm__("socket@12");
extern int WINAPI connect(SOCKET, const void *, int) __asm__("connect@12");
extern int WINAPI send(SOCKET, const char *, int, int) __asm__("send@16");
extern int WINAPI recv(SOCKET, char *, int, int) __asm__("recv@16");
extern int WINAPI closesocket(SOCKET) __asm__("closesocket@4");
extern int WINAPI WSACleanup(void) __asm__("WSACleanup@0");
extern void WINAPI SetLastError(DWORD) __asm__("SetLastError@4");
extern DWORD WINAPI GetLastError(void) __asm__("GetLastError@0");
extern BOOL WINAPI SetEvent(HANDLE) __asm__("SetEvent@4");
extern DWORD WINAPI GetFinalPathNameByHandleW(HANDLE, WCHAR *, DWORD, DWORD) __asm__("GetFinalPathNameByHandleW@16");
extern HANDLE WINAPI GetProcessHeap(void) __asm__("GetProcessHeap@0");
extern void *WINAPI HeapAlloc(HANDLE, DWORD, ULONG) __asm__("HeapAlloc@12");
extern BOOL WINAPI HeapFree(HANDLE, DWORD, void *) __asm__("HeapFree@12");
extern HANDLE WINAPI CreateThread(void *, ULONG, void *, void *, DWORD, DWORD *) __asm__("CreateThread@24");
extern BOOL WINAPI CloseHandle(HANDLE) __asm__("CloseHandle@4");
extern DWORD WINAPI WaitForSingleObject(HANDLE, DWORD) __asm__("WaitForSingleObject@8");
extern void WINAPI Sleep(DWORD) __asm__("Sleep@4");

struct sockaddr_in { USHORT family, port; ULONG address; unsigned char zero[8]; };
struct async_transfer { ULONG id; UCHAR op, pipe; PUCHAR buffer; ULONG length; PULONG transferred; LPOVERLAPPED overlapped; };

static int send_all(SOCKET s, const unsigned char *p, ULONG n)
{
    while (n)
    {
        int ret = send(s, (const char *)p, (int)n, 0);
        if (ret <= 0) return 0;
        p += ret; n -= ret;
    }
    return 1;
}
static int recv_all(SOCKET s, unsigned char *p, ULONG n)
{
    while (n)
    {
        int ret = recv(s, (char *)p, (int)n, 0);
        if (ret <= 0) return 0;
        p += ret; n -= ret;
    }
    return 1;
}
static void put32(unsigned char *p, ULONG v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static ULONG get32(const unsigned char *p) { return (ULONG)p[0] | ((ULONG)p[1] << 8) | ((ULONG)p[2] << 16) | ((ULONG)p[3] << 24); }

static DWORD decode_error(UCHAR code)
{
    switch (code)
    {
        case 2: return ERROR_OPERATION_ABORTED;
        case 3: return ERROR_DEVICE_NOT_CONNECTED;
        case 4: return ERROR_ACCESS_DENIED;
        case 5: return ERROR_SEM_TIMEOUT;
        case 6: return ERROR_INVALID_PARAMETER;
        case 7: return ERROR_BUSY;
        case 8: return ERROR_NOT_ENOUGH_MEMORY;
        case 9: return ERROR_NOT_SUPPORTED;
        case 10: return ERROR_NOT_FOUND;
        case 11: return ERROR_INVALID_HANDLE;
        default: return ERROR_GEN_FAILURE;
    }
}

static SOCKET connect_bridge(void)
{
    struct sockaddr_in addr;
    SOCKET s = socket(2, 1, 6);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;
    addr.family = 2;
    addr.port = (USHORT)((47821u << 8) | (47821u >> 8));
    addr.address = 0x0100007f;
    addr.zero[0] = addr.zero[1] = addr.zero[2] = addr.zero[3] = 0;
    addr.zero[4] = addr.zero[5] = addr.zero[6] = addr.zero[7] = 0;
    if (connect(s, &addr, 16)) { closesocket(s); return INVALID_SOCKET; }
    return s;
}
static USHORT parse_hex4(const WCHAR *s)
{
    USHORT value = 0; int i;
    for (i = 0; i < 4; ++i)
    {
        WCHAR c = s[i]; unsigned int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return 0;
        value = (value << 4) | d;
    }
    return value;
}
static UCHAR parse_hex2(const WCHAR *s)
{
    USHORT n = parse_hex4((const WCHAR[]){s[0],s[1],'0','0',0});
    return (UCHAR)(n >> 8);
}
static USHORT find_id(const WCHAR *path, const WCHAR *key)
{
    unsigned int i, j;
    for (i = 0; path[i] && i < 250; ++i)
    {
        for (j = 0; key[j] && path[i + j]; ++j)
        {
            WCHAR a = path[i + j], b = key[j];
            if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
            if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
            if (a != b) break;
        }
        if (!key[j]) return parse_hex4(path + i + j);
    }
    return 0;
}
static UCHAR find_interface(const WCHAR *path, DWORD count)
{
    unsigned int i;
    for (i = 0; i + 4 < count && i < 250; ++i)
    {
        WCHAR a = path[i], b = path[i + 1];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a == 'm' && b == 'i' && path[i + 2] == '_') return parse_hex2(path + i + 3);
    }
    return 0;
}

BOOL WINAPI WinUsb_Initialize(HANDLE device, WINUSB_INTERFACE_HANDLE *interface)
{
    WCHAR path[256]; DWORD count;
    USHORT vid = 0, pid = 0; UCHAR iface = 0;
    unsigned char request[6], reply[5]; SOCKET s; unsigned char wsa[512];
    if (!interface) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    *interface = 0;
    if (WSAStartup(0x0202, wsa)) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    count = GetFinalPathNameByHandleW(device, path, 256, 0);
    if (count && count < 256)
    {
        vid = find_id(path, (const WCHAR[]){'v','i','d','_',0});
        pid = find_id(path, (const WCHAR[]){'p','i','d','_',0});
        iface = find_interface(path, count);
    }
    s = connect_bridge();
    if (s == INVALID_SOCKET) goto failed;
    request[0] = 'O'; request[1] = vid; request[2] = vid >> 8;
    request[3] = pid; request[4] = pid >> 8; request[5] = iface;
    if (!send_all(s, request, 6) || !recv_all(s, reply, 5)) { closesocket(s); goto failed; }
    if (reply[0]) { closesocket(s); WSACleanup(); SetLastError(decode_error(reply[0])); return FALSE; }
    closesocket(s);
    *interface = (HANDLE)get32(reply + 1);
    return TRUE;
failed:
    WSACleanup(); SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE;
}

static BOOL transfer(SOCKET s, ULONG id, UCHAR op, UCHAR pipe, PUCHAR buffer, ULONG length, PULONG transferred)
{
    unsigned char request[10], reply[5]; ULONG actual;
    request[0] = op; put32(request + 1, id); request[5] = pipe; put32(request + 6, length);
    if (!send_all(s, request, 10) || (op == 'W' && length && !send_all(s, buffer, length)) || !recv_all(s, reply, 5)) goto failed;
    actual = get32(reply + 1);
    if (reply[0]) { SetLastError(decode_error(reply[0])); return FALSE; }
    if (actual > length || (op == 'R' && actual && !recv_all(s, buffer, actual))) goto failed;
    if (transferred) *transferred = actual;
    return TRUE;
failed:
    SetLastError(ERROR_GEN_FAILURE); return FALSE;
}
BOOL WINAPI WinUsb_Free(WINUSB_INTERFACE_HANDLE handle)
{
    unsigned char request[5] = {'C',0,0,0,0}, reply; SOCKET s; ULONG id = (ULONG)handle;
    if (!id) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    put32(request + 1, id); s = connect_bridge();
    if (s == INVALID_SOCKET) { WSACleanup(); SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    if (!send_all(s, request, 5) || !recv_all(s, &reply, 1)) { closesocket(s); WSACleanup(); SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    closesocket(s); WSACleanup();
    if (reply) { SetLastError(decode_error(reply)); return FALSE; }
    return TRUE;
}
static DWORD WINAPI transfer_worker(void *opaque)
{
    struct async_transfer *job = opaque;
    SOCKET s = connect_bridge(); ULONG actual = 0; BOOL ok = FALSE; DWORD error = ERROR_DEVICE_NOT_CONNECTED;
    if (s != INVALID_SOCKET) { ok = transfer(s, job->id, job->op, job->pipe, job->buffer, job->length, &actual); error = GetLastError(); closesocket(s); }
    if (job->transferred) *job->transferred = actual;
    job->overlapped->InternalHigh = actual;
    job->overlapped->Internal = ok ? 0 : error;
    __sync_synchronize();
    if (job->overlapped->hEvent) SetEvent((HANDLE)((ULONG)job->overlapped->hEvent & ~1u));
    HeapFree(GetProcessHeap(), 0, job);
    return 0;
}
static BOOL start_async(ULONG id, UCHAR op, UCHAR pipe, PUCHAR buffer, ULONG length, PULONG transferred, LPOVERLAPPED overlapped)
{
    struct async_transfer *job;
    HANDLE thread;
    if (!(job = HeapAlloc(GetProcessHeap(), 0, sizeof(*job)))) { SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    job->id = id; job->op = op; job->pipe = pipe; job->buffer = buffer; job->length = length;
    job->transferred = transferred; job->overlapped = overlapped;
    overlapped->InternalHigh = 0; overlapped->Internal = 0x103;
    if (!(thread = CreateThread(NULL, 0, transfer_worker, job, 0, NULL)))
    {
        HeapFree(GetProcessHeap(), 0, job); overlapped->Internal = ERROR_GEN_FAILURE;
        SetLastError(ERROR_GEN_FAILURE); return FALSE;
    }
    CloseHandle(thread); SetLastError(997); return FALSE;
}
BOOL WINAPI WinUsb_ReadPipe(WINUSB_INTERFACE_HANDLE h, UCHAR p, PUCHAR b, ULONG n, PULONG t, LPOVERLAPPED o)
{
    SOCKET s; ULONG actual = 0; BOOL ok;
    if (o) return start_async((ULONG)h, 'R', p, b, n, t, o);
    s = connect_bridge(); if (s == INVALID_SOCKET) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    ok = transfer(s, (ULONG)h, 'R', p, b, n, &actual); closesocket(s); if (t) *t = actual; return ok;
}
BOOL WINAPI WinUsb_WritePipe(WINUSB_INTERFACE_HANDLE h, UCHAR p, PUCHAR b, ULONG n, PULONG t, LPOVERLAPPED o)
{
    SOCKET s; ULONG actual = 0; BOOL ok;
    if (o) return start_async((ULONG)h, 'W', p, b, n, t, o);
    s = connect_bridge(); if (s == INVALID_SOCKET) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    ok = transfer(s, (ULONG)h, 'W', p, b, n, &actual); closesocket(s); if (t) *t = actual; return ok;
}
BOOL WINAPI WinUsb_AbortPipe(WINUSB_INTERFACE_HANDLE h, UCHAR p)
{
    unsigned char request[6] = {'A',0,0,0,0,p}, reply; SOCKET s = connect_bridge();
    if (s == INVALID_SOCKET) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    put32(request + 1, (ULONG)h);
    if (!send_all(s, request, 6) || !recv_all(s, &reply, 1)) { closesocket(s); SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    closesocket(s); if (reply) { SetLastError(decode_error(reply)); return FALSE; } return TRUE;
}
BOOL WINAPI WinUsb_GetOverlappedResult(WINUSB_INTERFACE_HANDLE h, LPOVERLAPPED o, PULONG t, BOOL wait)
{
    DWORD result;
    (void)h;
    if (!o) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    if (o->Internal == 0x103)
    {
        if (!wait) { SetLastError(996); return FALSE; }
        if (o->hEvent) result = WaitForSingleObject((HANDLE)((ULONG)o->hEvent & ~1u), 0xffffffffu);
        else { while (o->Internal == 0x103) Sleep(1); result = 0; }
        if (result) { SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    }
    __sync_synchronize();
    if (o->Internal) { SetLastError(o->Internal); return FALSE; }
    if (t) *t = o->InternalHigh;
    return TRUE;
}
BOOL WINAPI WinUsb_GetDescriptor(WINUSB_INTERFACE_HANDLE h, UCHAR type, UCHAR index, USHORT lang, PUCHAR b, ULONG n, PULONG t)
{
    unsigned char request[13], reply[5]; SOCKET s = connect_bridge(); ULONG actual;
    if (s == INVALID_SOCKET) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    request[0] = 'D'; put32(request + 1, (ULONG)h); request[5] = type; request[6] = index;
    request[7] = lang; request[8] = lang >> 8; put32(request + 9, n);
    if (!send_all(s, request, 13) || !recv_all(s, reply, 5)) goto failed;
    actual = get32(reply + 1);
    if (reply[0]) { closesocket(s); SetLastError(decode_error(reply[0])); return FALSE; }
    if (actual > n || (actual && !recv_all(s, b, actual))) goto failed;
    closesocket(s); if (t) *t = actual; return TRUE;
failed:
    closesocket(s); SetLastError(ERROR_GEN_FAILURE); return FALSE;
}
