# Test results

Tested on 2026-11-15, 2026-11-23, and 2026-11-24 with Logger Pro 3.16.2 under Wine 11.21 on Linux. This is a record of observed behavior on one setup, not a claim of full compatibility.

## Verified in Logger Pro

- The application launched through the installed Bottles/Wine setup; File, Edit, Experiment, Data, Analyze, Insert, Options, and Page menus rendered and responded.
- The bundled Basketball Shot analysis sample opened, its linked video played and advanced through all 75 frames, and graph/table/tracked-point content rendered.
- The sample could be saved and reopened after relinking its video to a local path. Statistics matched an independent calculation from the sample data.
- Table entry, calculated columns, text import/export, graph/table options, histogram/FFT/strip chart, fit/statistics tools, interpolation, integration/tangent/peak tools, prediction, Data Browser, and User Parameters were exercised.
- Multi-page add/delete and page options worked. Rectangle and oval insertion, multi-object selection, left alignment, layer ordering, graph grouping, common-Y grouping, and ungrouping worked on an unsaved test page.
- In the installed sample, the toolbar's Next Page and Previous Page buttons navigated between “1: Position” and “2: Velocity”; Zoom In and Zoom Out changed and restored the graph scale. Import From TI Device opened its port/device dialog. With no calculator attached, **Scan for Device** returned “Could not find device.”
- File → Print Preview rendered the current page, including its video, graph, text, and data table. A separate File → Print run with **Print to file** enabled produced a PostScript file; Ghostscript converted it to a two-page A4 PDF whose first page showed the sample video, graph, text, and table. No CUPS job was queued, and the physical printer was not used.
- Preferences, Accessibility Settings, Data Sharing, Printing Options, and Page Setup dialogs opened and responded. Data Sharing showed that its service was not running and was left disabled. Remote Save As exposed a LabArchives submenu; no external save was attempted. LabQuest Browser was unavailable without a connected LabQuest.
- With the virtual Go!Link connected, Experiment → Set Up Sensors showed the Go! interface, firmware/serial information, and its Voltage (+/-10 V) sensor. Experiment → Data Collection opened with Time Based, 15 seconds, 10 samples/second (151 samples), and triggering disabled; the dialog was canceled without changing the test experiment.
- Experiment → Zero on the virtual interface changed the live potential readout from about 3.35 V to about 0.10 V. The Experiment → Calibrate → Go!Link item opened the calibration dialog with sensor info, current calibration, units, and two reference-reading fields. Calibration was canceled without applying points. This checks the UI/protocol response only; zero/calibration accuracy on a physical sensor is unverified.
- With the Basketball Shot sample open, File → Accessibility → Settings opened the Audio Graph Trace controls with both video-analysis traces selected. Start Playback emitted non-silent audio to a temporary null sink; Audio Feedback was left off, and no audio went to the physical speaker.
- On 2026-11-24, Logger Pro ran on the project's 1920×1080 Xvfb display with a copy of `LoggerPro-validation.cmbl`. The Next Page toolbar button changed the page from Position to Velocity; Previous Page returned to Position. Zoom In changed the graph range and Zoom Out restored it. The Experiment menu rendered, and collection remained unavailable with no connected sensor. The test copy was not saved.

## Observed compatibility issue

- Help → Logger Pro Help reproducibly displayed “HelpIntro.html Not Found” under Wine 11.21, both when Logger Pro's working directory was the install root and when it was `en-US`. The CHM archive contains `HelpIntro.html`, and opening the archive directly with Wine's `hh.exe` displayed its contents. The help archive is present; the failure appears in Logger Pro's integrated Help path under Wine, but the precise cause is unresolved.

One Analyze sequence showed an internal application error after canceling Peak Integration and reopening Analyze. The dialog reported `GXSTextPen::GXSTextPen` in `GXSTextPen_Win.cpp`, line 648. The same error dialog appeared during a preliminary Xvfb interaction where the active modal state was unclear. After a clean Wine-server restart, the sample printed successfully to a local PostScript file without reproducing it. The trigger remains unresolved.

## Verified in the virtual USB harness

`winusb-bridge/virtual_usb_test.sh` builds and runs the native PE32 WinUSB implementation plus loopback bridge against a Go!Link-shaped virtual libusb device. It passes device descriptor retrieval, bulk write/read, interrupt-pipe cancellation, and interface release. Separate probes cover initialize/free, overlapped completion/event signaling, and the no-device Win32 error result. It passed again on 2026-11-24.

This checks the bridge and API implementation. It does not emulate USB at the Linux kernel/PnP layer or implement Vernier's sensor protocol.

On 2026-11-24, `winusb-bridge/virtual_golink_test.sh` built the Linux UHID emulator and passed packet-level checks for status/INIT, the 20-byte flash record split across three reports, sensor ID 2, analog channel selection, measurement period, LED/serial responses, start/stop, and measurement packet format. This validates the emulator's response builder, not Logger Pro's use of it.

On 2026-11-24, the emulator created a kernel HID device with `HID_ID=0003:000008F7:00000003`, and Ctrl-C removed it cleanly. Initially, `/dev/hidraw0` was `root:root` mode `0600` and inaccessible to the test user. After installing `udev/99-loggerpro-vernier.rules` and reloading udev, a recreated device appeared as `root:wheel` mode `0660`; a read/write open by the test user succeeded, and `udevadm verify` accepted the installed rule.

On 2026-11-23, a second test path ran Logger Pro under Xvfb with Wine's real `wineusb.so` loading a fake `08f7:0003` libusb device. Wine logged hotplug arrival, read the descriptor, and opened it. A temporary HID registry interface then made **Experiment → Connect Interface → Go!Link → Go!Link USB** available in Logger Pro. Selecting it displayed “Could not connect”; after dismissing that dialog, the File menu still opened and the app remained responsive. The registry entry is discovery-only, not a live HID device, and the fake USB backend does not implement the GoIO sensor protocol.

## Not verified

- On 2026-11-24, Logger Pro 3.16.2 was launched in Wine with the protocol-aware UHID Go!Link emulator and installed udev rule. Wine enumerated `HID\VID_08F7&PID_0003`; Logger Pro opened Go!Link, recognized its Voltage (+/-10 V) sensor, and displayed the Go! live readout. Starting collection populated the time/potential table and graph with a changing synthetic signal; pressing Stop ended collection, returned the toolbar to Collect, and left the collected dataset visible. The data-collection settings dialog also opened with the interface connected. Removing the virtual device disabled collection while Logger Pro remained responsive; recreating the device restored the live readout and Collect control. This verifies the USB/HID and application acquisition path against the emulator, including collection stop and disconnect/reconnect behavior. Values are synthetic, not real sensor readings; physical-device behavior and sensor-specific calibration remain unverified.
- No physical Vernier (`08f7`) or Ocean Optics (`2457`) device was connected during testing.
- Linux detected an integrated USB UVC camera (`/dev/video0`, 1280×720 MJPG at 30 fps). Logger Pro's Insert → Video Capture source chooser listed three microphone sources but no clearly identified camera source; the chooser was canceled without activating a microphone. End-to-end camera capture is still unverified.
- Opening the Go!Link USB menu item against the registry-only mock failed to connect; the status remained “No device connected.”
- Device detection against real hardware, sensor-specific calibration/zero accuracy, actual sensor values, USB transfer timing, and cancellation on a real instrument remain unverified.
- TI-device scanning with an attached calculator, LabQuest Browser with a LabQuest, Remote Save/remote collection, camera capture, Data Sharing with a client, accessibility embossing, and physical printer output remain unverified end to end. Data Sharing was left disabled; the print test wrote to a local file instead of sending a job.
- Bluetooth devices were explicitly out of scope.
