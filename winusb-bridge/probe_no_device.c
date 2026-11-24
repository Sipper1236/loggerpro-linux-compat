typedef unsigned char UCHAR;
typedef unsigned int ULONG;
typedef int BOOL;
typedef void *HANDLE;
typedef unsigned char *PUCHAR;
typedef unsigned int *PULONG;
typedef HANDLE WINUSB_INTERFACE_HANDLE;
typedef struct { ULONG Internal, InternalHigh, Offset, OffsetHigh; HANDLE hEvent; } OVERLAPPED;
typedef OVERLAPPED *LPOVERLAPPED;
#define WINAPI __attribute__((stdcall))
extern BOOL WINAPI WinUsb_Initialize(HANDLE, WINUSB_INTERFACE_HANDLE *) __asm__("WinUsb_Initialize@8");
extern void WINAPI ExitProcess(unsigned int) __asm__("ExitProcess@4");
extern unsigned int WINAPI GetLastError(void) __asm__("GetLastError@0");
void WINAPI test_entry(void)
{
    WINUSB_INTERFACE_HANDLE h = 0;
    if (WinUsb_Initialize((HANDLE)1, &h)) ExitProcess(1);
    if (GetLastError() != 1167) ExitProcess(2);
    ExitProcess(0);
}
