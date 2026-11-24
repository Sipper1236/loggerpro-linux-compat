# Test results

Tested on 2026-11-15 and 2026-11-23 with Logger Pro 3.16.2 under Wine 11.21 on Linux. This is a record of observed behavior on one setup, not a claim of full compatibility.

## Verified in Logger Pro

- The application launched through the installed Bottles/Wine setup; File, Edit, Experiment, Data, Analyze, Insert, Options, and Page menus rendered and responded.
- The bundled Basketball Shot analysis sample opened, its linked video played and advanced through all 75 frames, and graph/table/tracked-point content rendered.
- The sample could be saved and reopened after relinking its video to a local path. Statistics matched an independent calculation from the sample data.
- Table entry, calculated columns, text import/export, graph/table options, histogram/FFT/strip chart, fit/statistics tools, interpolation, integration/tangent/peak tools, prediction, Data Browser, and User Parameters were exercised.
- Multi-page add/delete and page options worked. Rectangle and oval insertion, multi-object selection, left alignment, layer ordering, graph grouping, common-Y grouping, and ungrouping worked on an unsaved test page.
- Sensor setup and data-collection dialogs opened. The triggering controls were unavailable without a connected sensor interface.

One Analyze sequence once showed an internal application error in `GXSTextPen.cpp` after canceling Peak Integration and reopening Analyze. It was not reproduced after relaunch; treat it as an observed compatibility issue.

## Verified in the virtual USB harness

`winusb-bridge/virtual_usb_test.sh` builds and runs the native PE32 WinUSB implementation plus loopback bridge against a Go!Link-shaped virtual libusb device. It passes device descriptor retrieval, bulk write/read, interrupt-pipe cancellation, and interface release. Separate probes cover initialize/free, overlapped completion/event signaling, and the no-device Win32 error result.

This checks the bridge and API implementation. It does not emulate USB at the Linux kernel/PnP layer or implement Vernier's sensor protocol.

## Not verified

- No Vernier (`08f7`) or Ocean Optics (`2457`) device was connected during testing.
- Logger Pro's app-level path did not issue WinUSB requests to the virtual bridge and continued to report “No device connected.”
- Device detection, sensor configuration against hardware, calibration/zero, collection, actual sample values, USB transfer timing, and cancellation on a real instrument remain unverified.
- Bluetooth devices were explicitly out of scope.
