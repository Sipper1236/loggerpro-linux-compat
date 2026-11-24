typedef unsigned char UCHAR;
typedef unsigned int ULONG;
typedef int BOOL;
typedef void *HANDLE;
typedef unsigned char *PUCHAR;
typedef unsigned int *PULONG;
typedef HANDLE WINUSB_INTERFACE_HANDLE;
typedef struct { volatile ULONG Internal, InternalHigh; ULONG Offset, OffsetHigh; HANDLE hEvent; } OVERLAPPED;
typedef OVERLAPPED *LPOVERLAPPED;
#define WINAPI __attribute__((stdcall))
#define NULL ((void *)0)
extern BOOL WINAPI WinUsb_Initialize(HANDLE, WINUSB_INTERFACE_HANDLE *) __asm__("WinUsb_Initialize@8");
extern BOOL WINAPI WinUsb_Free(WINUSB_INTERFACE_HANDLE) __asm__("WinUsb_Free@4");
extern BOOL WINAPI WinUsb_ReadPipe(WINUSB_INTERFACE_HANDLE,UCHAR,PUCHAR,ULONG,PULONG,LPOVERLAPPED) __asm__("WinUsb_ReadPipe@24");
extern BOOL WINAPI WinUsb_WritePipe(WINUSB_INTERFACE_HANDLE,UCHAR,PUCHAR,ULONG,PULONG,LPOVERLAPPED) __asm__("WinUsb_WritePipe@24");
extern BOOL WINAPI WinUsb_GetOverlappedResult(WINUSB_INTERFACE_HANDLE,LPOVERLAPPED,PULONG,BOOL) __asm__("WinUsb_GetOverlappedResult@16");
extern void WINAPI ExitProcess(unsigned int) __asm__("ExitProcess@4");
extern HANDLE WINAPI CreateEventW(void *,BOOL,BOOL,const unsigned short *) __asm__("CreateEventW@16");
extern unsigned int WINAPI GetLastError(void) __asm__("GetLastError@0");
extern BOOL WINAPI CloseHandle(HANDLE) __asm__("CloseHandle@4");
void WINAPI test_entry(void)
{
    WINUSB_INTERFACE_HANDLE h = 0;
    UCHAR out[4] = {1,2,3,4}, in[4] = {0,0,0,0};
    ULONG n = 0; OVERLAPPED ov = {0,0,0,0,0};
    if (!WinUsb_Initialize((HANDLE)1, &h)) ExitProcess(1);
    ov.hEvent = CreateEventW(NULL, 1, 0, NULL);
    if (!ov.hEvent) ExitProcess(2);
    if (WinUsb_WritePipe(h, 0x01, out, 4, &n, &ov) || GetLastError() != 997) ExitProcess(3);
    if (!WinUsb_GetOverlappedResult(h, &ov, &n, 1) || n != 4) ExitProcess(4);
    CloseHandle(ov.hEvent);
    ov = (OVERLAPPED){0,0,0,0,0}; ov.hEvent = CreateEventW(NULL, 1, 0, NULL);
    if (!ov.hEvent) ExitProcess(5);
    n = 0;
    if (WinUsb_ReadPipe(h, 0x81, in, 4, &n, &ov) || GetLastError() != 997) ExitProcess(6);
    if (!WinUsb_GetOverlappedResult(h, &ov, &n, 1) || n != 4) ExitProcess(7);
    if (in[0] != 0xde || in[1] != 0xad || in[2] != 0xbe || in[3] != 0xef) ExitProcess(8);
    CloseHandle(ov.hEvent);
    if (!WinUsb_Free(h)) ExitProcess(9);
    ExitProcess(0);
}
