# Test results

Tested on 2026-11-15 and 2026-11-23 with Logger Pro 3.16.2 under Wine 11.21 on Linux. This is a record of observed behavior on one setup, not a claim of full compatibility.

## Verified in Logger Pro

- The application launched through the installed Bottles/Wine setup; File, Edit, Experiment, Data, Analyze, Insert, Options, and Page menus rendered and responded.
- The bundled Basketball Shot analysis sample opened, its linked video played and advanced through all 75 frames, and graph/table/tracked-point content rendered.
- The sample could be saved and reopened after relinking its video to a local path. Statistics matched an independent calculation from the sample data.
- Table entry, calculated columns, text import/export, graph/table options, histogram/FFT/strip chart, fit/statistics tools, interpolation, integration/tangent/peak tools, prediction, Data Browser, and User Parameters were exercised.
- Multi-page add/delete and page options worked. Rectangle and oval insertion, multi-object selection, left alignment, layer ordering, graph grouping, common-Y grouping, and ungrouping worked on an unsaved test page.
- In the installed sample, the toolbar's Next Page and Previous Page buttons navigated between “1: Position” and “2: Velocity”; Zoom In and Zoom Out changed and restored the graph scale. The Import From TI Device button opened its port/device dialog. No TI device was connected, so scanning was not tested.
- File → Print Preview rendered the current page, including its video, graph, text, and data table. The preview was canceled without sending a print job.
- Preferences, Accessibility Settings, Data Sharing, Printing Options, and Page Setup dialogs opened and responded. Data Sharing showed that its service was not running and was left disabled. A configured printer appeared as ready, but no print job was sent.
- Sensor setup and data-collection dialogs opened. The triggering controls were unavailable without a connected sensor interface.

## Observed compatibility issue

- Help → Logger Pro Help reproducibly displayed “HelpIntro.html Not Found” under Wine 11.21, both when Logger Pro's working directory was the install root and when it was `en-US`. The CHM archive contains `HelpIntro.html`, and opening the archive directly with Wine's `hh.exe` displayed its contents. The help archive is present; the failure appears in Logger Pro's integrated Help path under Wine, but the precise cause is unresolved.

One Analyze sequence once showed an internal application error in `GXSTextPen.cpp` after canceling Peak Integration and reopening Analyze. It was not reproduced after relaunch; treat it as an observed compatibility issue.

## Verified in the virtual USB harness

`winusb-bridge/virtual_usb_test.sh` builds and runs the native PE32 WinUSB implementation plus loopback bridge against a Go!Link-shaped virtual libusb device. It passes device descriptor retrieval, bulk write/read, interrupt-pipe cancellation, and interface release. Separate probes cover initialize/free, overlapped completion/event signaling, and the no-device Win32 error result.

This checks the bridge and API implementation. It does not emulate USB at the Linux kernel/PnP layer or implement Vernier's sensor protocol.

On 2026-11-23, a second test path ran Logger Pro under Xvfb with Wine's real `wineusb.so` loading a fake `08f7:0003` libusb device. Wine logged hotplug arrival, read the descriptor, and opened it. A temporary HID registry interface then made **Experiment → Connect Interface → Go!Link → Go!Link USB** available in Logger Pro. Selecting it displayed “Could not connect”; after dismissing that dialog, the File menu still opened and the app remained responsive. The registry entry is discovery-only, not a live HID device, and the fake USB backend does not implement the GoIO sensor protocol.

## Not verified

- No Vernier (`08f7`) or Ocean Optics (`2457`) device was connected during testing.
- Opening the Go!Link USB menu item against the registry-only mock failed to connect; the status remained “No device connected.”
- Device detection against real hardware, sensor configuration, calibration/zero, collection, actual sample values, USB transfer timing, and cancellation on a real instrument remain unverified.
- TI-device scanning, LabQuest Browser, Remote Save/remote collection, camera or video capture, Data Sharing with a client, accessibility playback/embossing, and actual printer output were not exercised end to end. Data Sharing was left disabled; no service or print job was started.
- Bluetooth devices were explicitly out of scope.
