# Logger Pro Linux compatibility experiments

Experiments for running Vernier Logger Pro 3 on Linux with Wine. The project contains a startup compatibility shim and an experimental USB bridge with a virtual-device test harness. It is not affiliated with Vernier Software & Technology.

## Status

Logger Pro 3.16.2 launched and its non-hardware workflows were exercised on Linux with Wine 11.21. A virtual USB probe passes at the WinUSB/libusb bridge boundary. **Logger Pro has not collected data from a real instrument, and its app-level path did not issue WinUSB requests to the simulated device.** Treat the USB bridge as experimental and unverified for instrument use.

The Bluetooth shim only prevents an unsupported BLE watcher from crashing startup. It reports no discovered devices; Bluetooth sensor support is not provided.

See [test results](docs/test-results.md) for what was exercised and what remains unverified.

## Contents

- `loggerpro_bluetooth.c`, `build.sh`: build the 32-bit WinRT compatibility DLL.
- `winusb-bridge/`: native WinUSB DLL, loopback libusb helper, virtual libusb device, and probes.
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

The root build writes a generated DLL and assembly/object intermediates into this directory. The USB build writes generated files under `winusb-bridge/build/`; these are ignored by Git.

## Important limits

- Real Vernier and Ocean Optics hardware collection has not been tested.
- The Logger Pro UI did not send requests to the simulated USB bridge, so the virtual test does not verify Logger Pro device detection or collection.
- The bridge supports only the listed Logger Pro-era USB IDs and Ocean Optics IDs in its source; it does not support every Go Direct device.
- Endpoint selection assumes alternate setting 0. Cancellation, endpoint behavior, and timing still need hardware validation.
- The Bluetooth shim disables BLE discovery rather than emulating sensors.

Do not rely on this prototype for lab measurements until it has been validated with the actual supported instrument and sensor.

## License

No license is included. All rights are reserved by default.
