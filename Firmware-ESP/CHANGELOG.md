## Change Log

### V1.0.0
- **Initial fork with Matt Freyer's changes incorporated**

### V1.0.1
- **Minor fixes to allow code to compile under latest VS Code and PlatformIO**
- **Tested and deployed to a working environment**

### V1.1
- **No new features.** This release fixes crashes, buffer overflows and parsing bugs, and corrects spelling. The WiSafe2 protocol behaviour is unchanged.
- **Crashes and memory safety**
  - Fixed a crash when a device is removed from Home Assistant. Clearing a retained discovery config published a `NULL` payload, which called `strlen(NULL)`. It now publishes an empty retained payload.
  - Fixed an uptime buffer overflow. The uptime string overflowed after about 27.8 hours. The buffer is larger and the format is correct.
  - Fixed a device ID buffer overflow. The Home Assistant device ID string overflowed for 8-digit device IDs.
  - Added a NULL check around the SQLite database. A failed SPIFFS mount or database open no longer crashes the gateway.
  - Added bounds checks in the message escape and unescape code, the TX buffer, the TX stop-word scan and the RX buffer.
  - Zero-padded short packets. Incoming packets shorter than expected are zero-padded and logged. Previously, stale bytes from the RX buffer were read.
- **Protocol and parsing**
  - Rewrote the UART frame parser.
    - A `{` (0x7B) byte inside a payload no longer restarts the frame.
    - Length bytes over 127 are no longer read as negative values.
    - Zero and oversized lengths are rejected.
    - A half-received frame is dropped after 200 ms so the parser can resynchronise.
  - A full TX queue now drops the new message and logs it. It used to overwrite an unsent one.
  - Fixed the SID map log output, which printed a 64-bit value with a 32-bit format.
- **MQTT and Home Assistant**
  - Fixed command topic matching. The topic comparison used the payload length. Topics and commands are now matched exactly, and oversized messages are ignored.
  - Fixed the `"None"` pointer comparison. Sensors no longer get a literal `"None"` as their device class and unit.
  - Discovery configs are no longer published if they were truncated. The JSON document is larger and an overflow is logged.
  - The MQTT client ID is now unique per gateway (`ws2mqtt_<device id>`), so several gateways can share one broker.
  - Home Assistant discovery is only resent when Home Assistant reports `online`, not on every status message.
  - WiFi now auto-reconnects, and MQTT waits for WiFi.
- **Code quality and spelling**
  - Fixed spelling: "Unknown", "Interrupt Watchdog" and the OTA comment.
  - Replaced unsafe `sprintf` calls with `snprintf` and used unsigned formats for device IDs.
  - Corrected the database callback function types.
  - `ownSid` is now `int` and defined in `main.cpp`. It was a `char` defined in a header.
  - Added missing headers, includes and a function declaration.
- **Upgrade notes**
  - Unknown-model devices were named "Unkown ..." in Home Assistant, and the name is now "Unknown ...". Existing entity IDs do not change, because the unique IDs are unchanged. Only the displayed names update.
- **No configuration changes are required.**
  - Your existing `credentials.h` still works.

### V1.2.0
- **Web interface on port 80** (no login yet). Open `http://<gateway IP>/` to see:
  - **WiSafe devices:** ID, SID, type, model, alarm state, battery and error flags, signal, and when each device was last heard from.
  - **Recent MQTT packets:** a log of the latest messages sent and received, with a filter that hides housekeeping traffic such as discovery and sensor updates.
  - **WiSafe module:** paired state, SID, device ID, battery, RSSI, flags, radio faults, SIDs on the network, the SID map, and UART receive and transmit counters.
  - **ESP32:** firmware version and build time, uptime, boot reason, chip model and revision, flash size, heap use, WiFi network and signal, IP, MAC, BSSID, hostname, and MQTT connection statistics.
  - The page reads its data from JSON endpoints under `/data/` (`system.json`, `module.json`, `devices.json`, `mqtt.json`).
- **Firmware version**
  - The version is defined once in `include/version.h`.
  - It is shown on the web page and sent to Home Assistant as a new **Firmware Version** sensor and as the bridge device's software version.
- **Internal**
  - New in-memory device state and a ring buffer of recent MQTT messages feed the web interface.
  - Every MQTT publish now goes through one function that records it in the log and counts sends and failures.
- **Upgrade notes**
  - Your existing `credentials.h` still works.

### V1.3.0
- **Support for devices that showed as Unknown**
  - Device types and model codes are now looked up in one table (`device_models.cpp`), using the data the devices already report.
  - Added the **FP2620W2-R** smoke alarm (model `0x0E34`) and the **Alarm Control Unit** (type `0xC4`, model `0x0445`).
  - **Heat detectors** now get the Home Assistant `heat` device class and are named "Heat detector".
  - Control units get a generic "Alarm" entity. Their other entities are kept.
  - A model code that is not in the table is shown as `Unknown 0xNNNN`, so new models are easy to spot.
- **Bridge device sensors**
  - Every value shown on the web page is now also sent over MQTT and appears as a diagnostic sensor on the bridge device: 49 sensors covering the ESP32 (build, chip, CPU speed, flash, firmware size, heap, SDK, bridge name and ID), WiFi and MQTT (hostname, BSSID, channel, broker, connection and message counters, last connect error) and the WiSafe module (pairing, SIDs, battery, RSSI, flags, faults, SID map, traffic counters and TX queue).
  - Values are sent when they change, with a full refresh every 5 minutes and after an MQTT reconnect.
  - The sensors are all defined in one table in `bridge_sensors.cpp`, so adding another is one line.
- **Upgrade notes**
  - **Flash this version over USB once.** `platformio.ini` now uses the `min_spiffs.csv` partition table, which gives a larger app slot. The firmware had outgrown the default slot and OTA failed with a size error. After the USB flash, OTA works again. The change resets the device database, so devices are rediscovered. Their Home Assistant entities keep the same IDs.
  - Home Assistant entity names and device classes change for models that were unknown, and for heat detectors. Entity IDs and unique IDs do not change.
  - The bridge sensors `ip_addr`, `rssi`, `mac_addr` and `ssid` were renamed `wifi_ip_addr`, `wifi_rssi`, `wifi_mac_addr` and `wifi_ssid`. The old entities are removed automatically. Update any automation or dashboard that uses them.
  - If you have an alarm automation, add the heat detector to its alarm types, and remove the control unit `..._detector` entity from its triggers.

### V1.4.0
- **Known devices are asked for their status.**
  - After a reboot, devices already in the database showed an Unknown alarm state until an alarm message arrived. The gateway now sends a status query to any stored device whose state is not known yet, each time the SID map is received (about once a minute).
  - The reply fills in battery, signal and fault data, sets the alarm to off and publishes it to MQTT. It also recovers from a lost reply.
  - A device that already has a known state is never queried, so a real alarm is not overwritten. The radio protocol cannot report an alarm that is already sounding.
  - This is controlled by `QUERY_STATUS_FOR_KNOWN_DEVICES` in `include/config.h`. Comment it out to disable it.

