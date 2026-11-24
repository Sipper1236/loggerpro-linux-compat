# Logger Pro Linux compatibility experiments

Experiments for running Vernier Logger Pro 3 on Linux with Wine. The project contains a startup compatibility shim and an experimental USB bridge with a virtual-device test harness. It is not affiliated with Vernier Software & Technology.

## Status

Logger Pro 3.16.2 launched and many non-hardware workflows were exercised on Linux with Wine 11.21. Virtual tests cover the WinUSB/libusb bridge, Wine's USB enumeration/open path, and Go!Link protocol packet generation. With the included Vernier udev rule installed, a protocol-aware Linux UHID emulator was detected by Logger Pro; its Voltage (+/-10 V) sensor populated a live readout, graph, and table during collection, and the Stop control ended collection. Virtual unplug/reconnect also disabled then restored collection while the app stayed responsive. Real hardware collection remains unverified. The emulator's deterministic samples are for compatibility testing, not lab measurements.

The Bluetooth shim only prevents an unsupported BLE watcher from crashing startup. It reports no discovered devices; Bluetooth sensor support is not provided.

See [test results](docs/test-results.md) for what was exercised and what remains unverified.

## Contents

- `loggerpro_bluetooth.c`, `build.sh`: build the 32-bit WinRT compatibility DLL.
- `winusb-bridge/`: native WinUSB DLL, loopback libusb helper, virtual libusb and Go!Link HID devices, and probes.
- `winusb-bridge/run_loggerpro_headless.sh`: launch Logger Pro inside an Xvfb virtual display with test-only Wine USB and HID discovery mocks.
- `winusb-bridge/udev/`: optional, product-scoped access rules for supported Vernier and Ocean Optics devices.

This repository contains source and test harnesses only. It does not include Logger Pro binaries, Vernier drivers, user settings, compiled DLLs, or build output.

## Build and test

On Arch Linux, install the toolchain and libusb development package (`gcc`, `llvm`, `lld`, `binutils`, `libusb`, `pkgconf`, `python`, and Wine). Then:

```sh
./build.sh
cd winusb-bridge
./build.sh
./virtual_usb_test.sh
./virtual_golink_test.sh
```

The WinUSB test runs the 32-bit DLL through a loopback helper against a test-only Go!Link-shaped libusb device. It checks descriptor retrieval, bulk write/read, interrupt cancellation, and interface release. The Go!Link protocol test checks status/init, flash-record packetization, sensor ID and channel, period and status queries, LED/serial responses, measurement start/stop, and synthetic measurement packet format. Neither one by itself proves Logger Pro collection through HID or real sensor measurements.

The headless launcher also needs `xvfb-run` and `xauth`. To open Logger Pro on a headless X display and inject a synthetic device into Wine's USB stack:

```sh
cd winusb-bridge
WINEPREFIX=/path/to/loggerpro-prefix ./run_loggerpro_headless.sh \
  "/path/to/loggerpro-prefix/drive_c/Program Files (x86)/Vernier Software/Logger Pro 3/LoggerPro.exe"
```

The fake USB device appears to Wine as `08f7:0003` and produces an arrival/open record in `/tmp/loggerpro-usb-enumeration.log` by default. The launcher also adds a temporary HID discovery entry to the selected Wine prefix so the Go!Link USB menu appears; it removes that entry when Logger Pro exits. Xvfb creates an invisible 1920×1080 display for the GUI. It does not keep the computer awake or override the host's lid-close behavior; an idle inhibitor may not prevent lid-triggered suspend. This discovery-only fake backend does not implement the GoIO sensor protocol and selecting it produces a connection error.

For the protocol-aware Linux HID emulator, install the optional [Vernier udev rule](winusb-bridge/udev/99-loggerpro-vernier.rules) and reload udev rules. Start `winusb-bridge/build/virtual_golink_uhid` in one terminal, then start Logger Pro with `LOGGERPRO_VIRTUAL_HID_REGISTRY=0` so Wine uses the kernel-enumerated HID device instead of the discovery-only registry entry. The emulator sends deterministic synthetic voltage samples; they are for compatibility testing, not lab measurements. On the test host, the rule made the emulator's hidraw node accessible to the test user. Logger Pro detected the Voltage sensor and completed visible graph/table collection through this path.

The root build writes a generated DLL and assembly/object intermediates into this directory. The USB build writes generated files under `winusb-bridge/build/`; these are ignored by Git.

## Important limits

- The protocol-aware UHID emulator has passed a visible Logger Pro collection: its Voltage (+/-10 V) sensor appeared, synthetic readings populated the graph/table, and the Stop control ended collection. Real instrument readings and sensor-specific calibration/zero remain unverified.
- Real Vernier and Ocean Optics hardware collection has not been tested.
- Linux sees the integrated USB camera, but Logger Pro's Video Capture source chooser showed only microphone inputs; camera capture remains unverified under Wine.
- Accessibility Audio Graph Trace playback emitted a signal to a temporary null sink with physical audio feedback disabled. Embossing output remains unverified.
- Logger Pro's Go!Link USB menu appears with the temporary discovery entry, but selecting that discovery-only mock fails to connect. The separate UHID protocol emulator communicates with Logger Pro; real hardware remains unverified.
- The bridge supports only the listed Logger Pro-era USB IDs and Ocean Optics IDs in its source; it does not support every Go Direct device.
- Endpoint selection assumes alternate setting 0. Cancellation, endpoint behavior, and timing still need hardware validation.
- The Bluetooth shim disables BLE discovery rather than emulating sensors.

Do not rely on this prototype for lab measurements until it has been validated with the actual supported instrument and sensor.

## License

No license is included. All rights are reserved by default.
