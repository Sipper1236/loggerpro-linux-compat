# Experimental Logger Pro USB bridge

This prototype replaces the Wine `winusb.dll` exports used by Logger Pro with a 32-bit native DLL. The DLL sends WinUSB calls over a loopback-only TCP connection to a Linux helper, which uses libusb to open supported Vernier USB interfaces (`08f7`) and Ocean Optics spectrometers (`2457`). It supports the Logger Pro imports observed in `LoggerPro.exe`, `NGIO_lib.dll`, and `SpectroVisSDK.dll`: initialize/free, bulk or interrupt read/write, abort, overlapped-result query, and descriptor read.

## Build and simulated check

Run `./build.sh`. It uses GCC, Wine's `winegcc` for the registry-only HID discovery helper, LLVM tools, and libusb development files. `mock.py` is a loopback server for the 32-bit `probe.exe`; it returns deterministic data so the DLL-to-host framing can be checked without hardware.

Run `./virtual_usb_test.sh` for the fuller virtual-device loop. It builds a test-only libusb backend that presents a Go!Link-shaped device (`08f7:0003`) and runs the real PE32 WinUSB DLL plus host bridge against it. It checks a device descriptor, async bulk write/read data, interrupt-pipe cancellation, and interface free. This emulates USB at the libusb boundary; it does not create a kernel-enumerated USB device or emulate the sensor's Vernier application protocol.

## Headless Logger Pro with Wine USB enumeration

`run_loggerpro_headless.sh` starts Logger Pro on an Xvfb display, places `wineusb_virtual.c` ahead of the system libusb library, and adds a temporary SetupAPI HID discovery entry to the selected Wine prefix. Wine's actual `wineusb.so` sees a synthetic Go!Link (`08f7:0003`), reads its USB descriptors, opens it, and reports an arrival. The registry entry makes Logger Pro's **Experiment → Connect Interface → Go!Link → Go!Link USB** menu item available. Run it with the installed prefix and executable path:

```sh
WINEPREFIX=/path/to/loggerpro-prefix ./run_loggerpro_headless.sh \
  "/path/to/loggerpro-prefix/drive_c/Program Files (x86)/Vernier Software/Logger Pro 3/LoggerPro.exe"
```

The default enumeration log is `/tmp/loggerpro-usb-enumeration.log`; override it with `LOGGERPRO_USB_ENUM_LOG`. The launcher removes its temporary registry entry when Logger Pro exits. The fake registry interface has no HID handle, and the USB mock does not implement Go!Link's sensor protocol; selecting Go!Link USB displays a connection error. This is a discovery/UI check only and must not be used for measurements. Xvfb supplies an invisible 1920×1080 X display, but it does not keep the host awake or override lid-close behavior. Check the host's lid policy; an idle inhibitor may not prevent lid-triggered suspend.

The Linux `bridge` binary listens only on `127.0.0.1:47821`. It accepts Vernier normal-mode IDs from the [official USB ID list](https://www.vernier.com/til/4289) for Logger Pro-era devices, including Go!Link and LabPro/LabQuest interfaces, plus Go Direct SpectroVis Plus (`0011`) and Ocean Optics `2457` spectrometers listed in Logger Pro's installed INFs. Bootloader IDs, LabQuest viewer-mode `000c` (RNDIS), and other Go Direct models are excluded; Logger Pro 3.16 documents USB support only for Go Direct SpectroVis Plus ([release notes](https://www2.vernier.com/manuals/LoggerPro3_16_ReleaseNotes.pdf)). When the interface path does not reveal a VID/PID, it selects the first matching supported device. Transfers use the endpoint byte supplied by Logger Pro and the bulk/interrupt type from alternate setting 0; alternate-setting changes are not implemented.

## Current proof and limitations

- The native DLL and helper built successfully on this machine without 32-bit system libraries.
- A 32-bit Windows test process completed initialize, write, read, overlapped-result retrieval, and free through `mock.py`.
- The real libusb helper returned the expected “no device” result when no supported instrument was attached; the Windows probe verifies `ERROR_DEVICE_NOT_CONNECTED` (`1167`).
- Logger Pro loaded this DLL as native, loaded `NGIO_lib.dll` and the Winsock dependency, and reached its normal single-instance UI path. The temporary DLL was removed from the installed application folder afterward.
- A follow-up mock probe passed worker-thread overlapped read/write completion and event signaling. The helper uses libusb asynchronous transfers and handles pipe cancellation requests; both components rebuild successfully. Cancellation has not been exercised against real USB hardware.
- The bridge now maps libusb open/transfer errors to useful Win32 errors, including access denied, disconnected, timeout, busy, unsupported, and aborted. The success mock and no-device error probes pass after this protocol change.
- `./virtual_usb_test.sh` passes against the test-only Go!Link-shaped device, including descriptor values `VID=08f7/PID=0003`, bulk data `de ad be ef`, and `ERROR_OPERATION_ABORTED` on the pending interrupt read.
- On 2026-11-23, Logger Pro displayed the Go!Link USB menu item with the temporary HID registry entry. Selecting it produced “Could not connect”; dismissing the dialog left the app responsive.
- A Logger Pro process loaded the app-local native `WINUSB.DLL`, `NGIO_lib.dll`, and `SpectroVisSDK.dll`. Its normal single-instance UI path was reached; the already-running stock instance was left open.
- **No real Vernier or Ocean Optics instrument has tested collection. This is not verified for normal use.** USB path parsing, interface selection, endpoint behavior, cancellation, and completion timing need hardware validation. If the path has no parseable VID/PID and multiple supported instruments are attached, the helper currently selects the first supported device. Endpoint lookup assumes alternate setting 0; changing alternate settings is not implemented. Transfers time out after 10 seconds.
- On 2026-11-23, a test-only `libusb-1.0.so.0` replacement caused Wine 11.21's real `wineusb.so` to enumerate and open fake Go!Link `08f7:0003`. The separate SetupAPI registry entry is not a real PnP HID device, so UI discovery does not demonstrate a working connection.

The development machine has a separate opt-in launcher that temporarily places this DLL in Logger Pro's application directory and starts the helper. That machine-specific launcher is not part of this repository. Do not install this prototype into a Logger Pro prefix for normal use until detection and collection have been validated with real hardware.

## USB permissions

The included rule grants the `wheel` group access to six supported Ocean Optics USB product IDs. Review it for your system before installing it:

```sh
sudo install -Dm644 udev/99-loggerpro-oceanoptics.rules /etc/udev/rules.d/99-loggerpro-oceanoptics.rules
sudo udevadm control --reload-rules
```

Reconnect the spectrometer after reloading the rules. This host-permission rule is optional and does not make Logger Pro recognize an instrument by itself.
