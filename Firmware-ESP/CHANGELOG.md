## Change Log

### V1.0.0
- **Initial fork with Matt Freyer's changes incorporated**

### V1.0.1
- **Minor fixes to allow code to compile under latest VS Code and PlatformIO**
- **Tested and deployed to a working environment**

### V1.1
- **No new features.** This release fixes crashes, buffer overflows and parsing bugs, and corrects spelling. The WiSafe2 protocol behaviour is unchanged.
- **Crashes and memory safety**
  - Fixed a crash when a device is removed from Home Assistant.** Clearing a retained discovery config published a `NULL` payload, which called `strlen(NULL)`. It now publishes an empty retained payload.
  - Fixed an uptime buffer overflow. The uptime string overflowed after about 27.8 hours. The buffer is larger and the format is correct.
  - Fixed a device ID buffer overflow. The Home Assistant device ID string overflowed for 8-digit device IDs.
  - Added a NULL check around the SQLite database. A failed SPIFFS mount or database open no longer crashes the gateway.
  - Added bounds checks** in the message escape and unescape code, the TX buffer, the TX stop-word scan and the RX buffer.
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
  - Discovery configs are no longer published if they were truncated.** The JSON document is larger and an overflow is logged.
  - The MQTT client ID is now unique per gateway** (`ws2mqtt_<device id>`), so several gateways can share one broker.
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

