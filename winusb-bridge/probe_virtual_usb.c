typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int ULONG;
typedef int BOOL;
typedef void *HANDLE;
typedef unsigned char *PUCHAR;
typedef unsigned int *PULONG;
typedef HANDLE WINUSB_INTERFACE_HANDLE;
typedef struct { ULONG Internal, InternalHigh, Offset, OffsetHigh; HANDLE hEvent; } OVERLAPPED;
typedef OVERLAPPED *LPOVERLAPPED;
#define WINAPI __attribute__((stdcall))
#define NULL ((void *)0)
extern BOOL WINAPI WinUsb_Initialize(HANDLE, WINUSB_INTERFACE_HANDLE *) __asm__("WinUsb_Initialize@8");
extern BOOL WINAPI WinUsb_Free(WINUSB_INTERFACE_HANDLE) __asm__("WinUsb_Free@4");
extern BOOL WINAPI WinUsb_ReadPipe(WINUSB_INTERFACE_HANDLE,UCHAR,PUCHAR,ULONG,PULONG,LPOVERLAPPED) __asm__("WinUsb_ReadPipe@24");
extern BOOL WINAPI WinUsb_WritePipe(WINUSB_INTERFACE_HANDLE,UCHAR,PUCHAR,ULONG,PULONG,LPOVERLAPPED) __asm__("WinUsb_WritePipe@24");
extern BOOL WINAPI WinUsb_AbortPipe(WINUSB_INTERFACE_HANDLE,UCHAR) __asm__("WinUsb_AbortPipe@8");
extern BOOL WINAPI WinUsb_GetOverlappedResult(WINUSB_INTERFACE_HANDLE,LPOVERLAPPED,PULONG,BOOL) __asm__("WinUsb_GetOverlappedResult@16");
extern BOOL WINAPI WinUsb_GetDescriptor(WINUSB_INTERFACE_HANDLE,UCHAR,UCHAR,USHORT,PUCHAR,ULONG,PULONG) __asm__("WinUsb_GetDescriptor@28");
extern void WINAPI ExitProcess(unsigned int) __asm__("ExitProcess@4");
extern HANDLE WINAPI CreateEventW(void *,BOOL,BOOL,const unsigned short *) __asm__("CreateEventW@16");
extern unsigned int WINAPI GetLastError(void) __asm__("GetLastError@0");
extern void WINAPI Sleep(unsigned int) __asm__("Sleep@4");
extern BOOL WINAPI CloseHandle(HANDLE) __asm__("CloseHandle@4");

void WINAPI test_entry(void)
{
    WINUSB_INTERFACE_HANDLE handle = 0;
    UCHAR descriptor[18] = {0}, out[4] = {1,2,3,4}, in[4] = {0};
    ULONG transferred = 0;
    OVERLAPPED overlapped = {0,0,0,0,0};
    unsigned int i;

    if (!WinUsb_Initialize((HANDLE)1, &handle)) ExitProcess(1);
    if (!WinUsb_GetDescriptor(handle, 1, 0, 0, descriptor, sizeof(descriptor), &transferred)) ExitProcess(2);
    if (transferred != 18 || descriptor[0] != 18 || descriptor[1] != 1 ||
        descriptor[8] != 0xf7 || descriptor[9] != 0x08 || descriptor[10] != 3) ExitProcess(3);

    overlapped.hEvent = CreateEventW(NULL, 1, 0, NULL);
    if (!overlapped.hEvent) ExitProcess(4);
    if (WinUsb_WritePipe(handle, 0x01, out, sizeof(out), &transferred, &overlapped) || GetLastError() != 997) ExitProcess(5);
    if (!WinUsb_GetOverlappedResult(handle, &overlapped, &transferred, 1) || transferred != sizeof(out)) ExitProcess(6);
    CloseHandle(overlapped.hEvent);

    overlapped = (OVERLAPPED){0,0,0,0,0};
    overlapped.hEvent = CreateEventW(NULL, 1, 0, NULL);
    if (!overlapped.hEvent) ExitProcess(7);
    transferred = 0;
    if (WinUsb_ReadPipe(handle, 0x81, in, sizeof(in), &transferred, &overlapped) || GetLastError() != 997) ExitProcess(8);
    if (!WinUsb_GetOverlappedResult(handle, &overlapped, &transferred, 1) || transferred != sizeof(in)) ExitProcess(9);
    if (in[0] != 0xde || in[1] != 0xad || in[2] != 0xbe || in[3] != 0xef) ExitProcess(10);
    CloseHandle(overlapped.hEvent);

    overlapped = (OVERLAPPED){0,0,0,0,0};
    overlapped.hEvent = CreateEventW(NULL, 1, 0, NULL);
    if (!overlapped.hEvent) ExitProcess(11);
    transferred = 0;
    if (WinUsb_ReadPipe(handle, 0x82, in, sizeof(in), &transferred, &overlapped) || GetLastError() != 997) ExitProcess(12);
    for (i = 0; i < 100; ++i)
    {
        unsigned int error;
        if (!WinUsb_AbortPipe(handle, 0x82)) ExitProcess(13);
        if (WinUsb_GetOverlappedResult(handle, &overlapped, &transferred, 0)) ExitProcess(14);
        error = GetLastError();
        if (error == 995) break;
        if (error != 996) ExitProcess(15);
        Sleep(10);
    }
    if (i == 100) ExitProcess(16);
    CloseHandle(overlapped.hEvent);
    if (!WinUsb_Free(handle)) ExitProcess(17);
    ExitProcess(0);
}
