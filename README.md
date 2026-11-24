# Logger Pro Linux compatibility experiments

Experiments for running Vernier Logger Pro 3 on Linux with Wine. The project contains a startup compatibility shim and an experimental USB bridge with a virtual-device test harness. It is not affiliated with Vernier Software & Technology.

## Status

Logger Pro 3.16.2 launched and its non-hardware workflows were exercised on Linux with Wine 11.21. Virtual tests cover the WinUSB/libusb bridge and Wine's real USB enumeration/open path. A separate temporary HID registry entry makes Logger Pro show its Go!Link USB menu item. Selecting it fails to connect, as expected: the registry entry is discovery-only and the USB mock does not implement Vernier's sensor protocol. **Sensor collection remains unverified, and no real instrument has been tested.**

The Bluetooth shim only prevents an unsupported BLE watcher from crashing startup. It reports no discovered devices; Bluetooth sensor support is not provided.

See [test results](docs/test-results.md) for what was exercised and what remains unverified.

## Contents

- `loggerpro_bluetooth.c`, `build.sh`: build the 32-bit WinRT compatibility DLL.
- `winusb-bridge/`: native WinUSB DLL, loopback libusb helper, virtual libusb device, and probes.
- `winusb-bridge/run_loggerpro_headless.sh`: launch Logger Pro inside an Xvfb virtual display with test-only Wine USB and HID discovery mocks.
- `winusb-bridge/udev/`: optional scoped access rule for supported Ocean Optics devices.

This repository contains source and test harnesses only. It does not include Logger Pro binaries, Vernier drivers, user settings, compiled DLLs, or build output.

## Build and test

On Arch Linux, install the toolchain and libusb development package (`gcc`, `llvm`, `lld`, `binutils`, `libusb`, `pkgconf`, `python`, and Wine). Then:

```sh
./build.sh
cd winusb-bridge
./build.sh
./virtual_usb_test.sh
```

The USB test runs the 32-bit WinUSB DLL through a loopback helper against a test-only Go!Link-shaped libusb device. It checks descriptor retrieval, bulk write/read, interrupt cancellation, and interface release. It does not create a kernel-enumerated USB device or emulate Vernier's sensor protocol. Set `WINEPREFIX` if you want the test to use a particular Wine prefix; otherwise it uses `~/.wine`.

The headless launcher also needs `xvfb-run` and `xauth`. To open Logger Pro on a headless X display and inject a synthetic device into Wine's USB stack:

```sh
cd winusb-bridge
WINEPREFIX=/path/to/loggerpro-prefix ./run_loggerpro_headless.sh \
  "/path/to/loggerpro-prefix/drive_c/Program Files (x86)/Vernier Software/Logger Pro 3/LoggerPro.exe"
```

The fake USB device appears to Wine as `08f7:0003` and produces an arrival/open record in `/tmp/loggerpro-usb-enumeration.log` by default. The launcher also adds a temporary HID discovery entry to the selected Wine prefix so the Go!Link USB menu appears; it removes that entry when Logger Pro exits. Xvfb creates an invisible 1920×1080 display for the GUI. It does not keep the computer awake or override the host's lid-close behavior; an idle inhibitor may not prevent lid-triggered suspend. The fake device does not generate Vernier sensor readings, and selecting Go!Link USB currently produces a connection error.

The root build writes a generated DLL and assembly/object intermediates into this directory. The USB build writes generated files under `winusb-bridge/build/`; these are ignored by Git.

## Important limits

- Real Vernier and Ocean Optics hardware collection has not been tested.
- Logger Pro's Go!Link USB menu appears with the temporary HID discovery entry, but selecting it fails to connect. Data collection and real hardware remain unverified.
- The bridge supports only the listed Logger Pro-era USB IDs and Ocean Optics IDs in its source; it does not support every Go Direct device.
- Endpoint selection assumes alternate setting 0. Cancellation, endpoint behavior, and timing still need hardware validation.
- The Bluetooth shim disables BLE discovery rather than emulating sensors.

Do not rely on this prototype for lab measurements until it has been validated with the actual supported instrument and sensor.

## License

No license is included. All rights are reserved by default.
