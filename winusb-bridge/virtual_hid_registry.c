#include <windows.h>
#include <setupapi.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const GUID hid_interface = {
    0x4d1e55b2, 0xf16f, 0x11cf, {0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}
};
static const WCHAR enum_path[] =
    L"System\\ControlSet001\\Enum\\HID\\VID_08F7&PID_0003\\LOGGERPRO-VIRTUAL-GOLINK";
static const WCHAR iface_path[] =
    L"System\\ControlSet001\\Control\\DeviceClasses\\{4D1E55B2-F16F-11CF-88CB-001111000030}\\##?#HID#VID_08F7&PID_0003#LOGGERPRO-VIRTUAL-GOLINK#{4D1E55B2-F16F-11CF-88CB-001111000030}";
static const WCHAR dev_instance[] = L"HID\\VID_08F7&PID_0003\\LOGGERPRO-VIRTUAL-GOLINK";
static const WCHAR symbolic_link[] =
    L"\\\\?\\HID#VID_08F7&PID_0003#LOGGERPRO-VIRTUAL-GOLINK#{4D1E55B2-F16F-11CF-88CB-001111000030}";

static int set_value(const WCHAR *path, const WCHAR *name, DWORD type,
                     const void *data, DWORD size)
{
    HKEY key;
    LONG result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, path, 0, NULL, 0,
                                 KEY_ALL_ACCESS, NULL, &key, NULL);
    if (result != ERROR_SUCCESS) {
        printf("RegCreateKeyEx failed (%ld)\n", result);
        return 0;
    }
    result = RegSetValueExW(key, name, 0, type, data, size);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        printf("RegSetValueEx failed (%ld)\n", result);
        return 0;
    }
    return 1;
}

static int install_device(void)
{
    static const WCHAR class_guid[] = L"{745A17A0-74D3-11D0-B6FE-00A0C90F57DA}";
    static const WCHAR class_name[] = L"HIDClass";
    static const WCHAR description[] = L"Vernier GoLink Virtual Test (registry discovery only)";
    static const WCHAR hardware_ids[] =
        L"HID\\VID_08F7&PID_0003&REV_0100\0HID\\VID_08F7&PID_0003\0";
    static const WCHAR compatible_ids[] = L"HID_DEVICE\0HID_DEVICE_SYSTEM\0";
    static const DWORD zero = 0, one = 1;
    WCHAR control_path[512], reference_path[512];
    lstrcpyW(control_path, iface_path);
    lstrcatW(control_path, L"\\#\\Control");
    lstrcpyW(reference_path, iface_path);
    lstrcatW(reference_path, L"\\#");

    if (!set_value(enum_path, L"ClassGUID", REG_SZ, class_guid, sizeof(class_guid)) ||
        !set_value(enum_path, L"Class", REG_SZ, class_name, sizeof(class_name)) ||
        !set_value(enum_path, L"DeviceDesc", REG_SZ, description, sizeof(description)) ||
        !set_value(enum_path, L"FriendlyName", REG_SZ, description, sizeof(description)) ||
        !set_value(enum_path, L"HardwareID", REG_MULTI_SZ, hardware_ids, sizeof(hardware_ids)) ||
        !set_value(enum_path, L"CompatibleIDs", REG_MULTI_SZ, compatible_ids, sizeof(compatible_ids)) ||
        !set_value(enum_path, L"ConfigFlags", REG_DWORD, &zero, sizeof(zero)) ||
        !set_value(iface_path, L"DeviceInstance", REG_SZ, dev_instance, sizeof(dev_instance)) ||
        !set_value(iface_path, L"SymbolicLink", REG_SZ, symbolic_link, sizeof(symbolic_link)) ||
        !set_value(control_path, L"Linked", REG_DWORD, &one, sizeof(one)) ||
        !set_value(reference_path, L"SymbolicLink", REG_SZ, symbolic_link, sizeof(symbolic_link)))
        return 0;

    puts("Installed synthetic Go!Link HID discovery entry in this Wine prefix.");
    return 1;
}

static void remove_key(const WCHAR *path)
{
    RegDeleteTreeW(HKEY_LOCAL_MACHINE, path);
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, path);
}

static void remove_device(void)
{
    remove_key(iface_path);
    remove_key(enum_path);
    puts("Removed synthetic Go!Link HID discovery entry from this Wine prefix.");
}

static int check_device(void)
{
    HDEVINFO devices = SetupDiGetClassDevsA(&hid_interface, NULL, NULL,
                                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs failed: %lu\n", GetLastError());
        return 0;
    }

    int found = 0;
    for (DWORD index = 0; ; ++index) {
        SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
        if (!SetupDiEnumDeviceInterfaces(devices, NULL, &hid_interface, index, &iface)) {
            if (GetLastError() != ERROR_NO_MORE_ITEMS)
                printf("SetupDiEnumDeviceInterfaces stopped with error %lu\n", GetLastError());
            break;
        }

        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailA(devices, &iface, NULL, 0, &required, NULL);
        SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = calloc(1, required);
        if (!detail) break;
        SP_DEVINFO_DATA device = {sizeof(device)};
        detail->cbSize = sizeof(*detail);
        if (SetupDiGetDeviceInterfaceDetailA(devices, &iface, detail, required, NULL, &device)) {
            char *path = detail->DevicePath;
            for (char *cursor = path; *cursor; ++cursor)
                *cursor = (char)tolower((unsigned char)*cursor);
            if (strstr(path, "hid#vid_08f7&pid_0003#loggerpro-virtual-golink")) {
                printf("SetupAPI enumerated synthetic HID path: %s\n", path);
                found = 1;
            }
        }
        free(detail);
    }

    SetupDiDestroyDeviceInfoList(devices);
    if (!found) puts("Synthetic Go!Link HID interface was not enumerated.");
    return found;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: virtual_hid_registry.exe install|check|remove\n");
        return 2;
    }
    if (!strcmp(argv[1], "install"))
        return install_device() && check_device() ? 0 : 1;
    if (!strcmp(argv[1], "check"))
        return check_device() ? 0 : 1;
    if (!strcmp(argv[1], "remove")) {
        remove_device();
        return 0;
    }
    fprintf(stderr, "Unknown action: %s\n", argv[1]);
    return 2;
}
