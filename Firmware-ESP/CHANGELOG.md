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

### V1.4.1
- **Status queries are sent one at a time.** Previously every query was sent 0.5 seconds apart, and several devices went unanswered. The next query is now sent when the previous one is answered, reported as unanswered, or times out after 6 seconds.
- **The module's "no answer" code is handled.** The radio module sends `0x46` when a device does not answer a query. The firmware used to ignore it. It is now counted against the device and logged.
- **Devices are asked for their status when they transmit.** If a test press or an error or dock message arrives from a device whose alarm state is still unknown, a status query is queued straight away, while the device is awake. This uses the existing `QUERY_STATUS_FOR_KNOWN_DEVICES` setting.
- **Web interface:** the "Last heard" column now shows status queries, replies, no-answers and the time of the last reply for each device, and `/data/devices.json` has a new `status_queries` object.
- **New files:** `include/status_query.h` and `src/status_query.cpp`.

### V1.4.2
- **Fixed the module's `46 7E` code being treated as "no answer".** It is the module's immediate acknowledgement of a command, and the real reply follows if the device answers. A query now ends when its reply arrives or after the 6 second timeout, and "no answer" is counted only on a timeout. V1.4.1 ended the query on the acknowledgement, so queries were not really sent one at a time and the no-answer counts were wrong.
- The acknowledgement is logged (`Module acknowledged query to SID n`).

### V1.4.3
- **Web interface favicon.** The browser tab now shows a small icon (a dot with radio waves on a teal square). It is an SVG served from the gateway at `/favicon.svg`, and `/favicon.ico` returns the same image, so browsers that ask for the old path still get it. The tab previously showed a blank icon.

### V1.4.4
- **Forget devices from the web interface.**
  - A **Forget** button on each device row, and a **Forget all devices** button above the table. Both ask for confirmation.
  - Forgetting a device removes its entities from Home Assistant (discovery configs and retained states), deletes it from the gateway's database and in-memory list, and cancels any queued status query for it.
  - The gateway then rescans the radio network straight away, so a device that is still paired and answering is found again and re-announced. Devices that have been replaced or no longer answer stay out.
  - New endpoints: `POST /api/forget?sid=<n>` and `POST /api/forget-all`. They only accept a request that carries an `X-Requested-With: ws2mqtt` header, which another website cannot add, so a page open in your browser cannot trigger them. There is still no login (that comes with the admin account in step 4).
  - New files: `include/device_manage.h` and `src/device_manage.cpp`.
- **Alarm Control Units now use their real model name, `WTSL-F-1EU`**, in the web interface and in Home Assistant. This renames the device and entity names; entity IDs and unique IDs do not change.

### V1.4.5
- **Probe devices from the web interface.** A **Probe** button on each device row and a **Probe all devices** button ask devices for their status straight away (a single probe goes to the front of the queue). The reply updates the device's battery, signal and fault data and the "status replies" counters.
- **A probe never clears a real alarm.** A status reply now sets the alarm to off only when the alarm state was not already known, so probing a device that is sounding leaves it in alarm.
- **Raw status reply shown.** The "Last heard" cell shows the raw bytes of the last status reply (`raw 07 67 84 ...`), and `/data/devices.json` has them as `status_queries.raw`. This is to find out whether the reply says anything about the docked state, which is otherwise only reported when a device sends an event.
- **Bug fix (since V1.4.1):** the status reply handler did not tell the query queue that the reply had arrived. The call had been placed in the alarm-off handler by mistake. Queries therefore only moved on after the 6 second timeout, and the "status replies" counters never counted. Fixed.
- New endpoints `POST /api/probe?sid=<n>` and `POST /api/probe-all`, protected in the same way as the forget endpoints.

### V1.4.6
- **Device location.**
  - A new editable **Location** column in the web interface (for example "Kitchen"). It saves when you leave the field or press Enter, and the table does not refresh while you are typing. The location can be up to 32 characters. Clear the field to remove it.
  - The location is sent to Home Assistant as a new diagnostic **Location** sensor on each device, and as the device's suggested area. Home Assistant only uses the suggested area when it first creates the device, so the Location sensor is the reliable way to use it later.
  - Locations are stored by device ID in a new `locations` table in the gateway database. They survive **Forget** and rediscovery, and are cleared only when the radio module is unpaired and the database is reset.
  - New endpoint `POST /api/location?sid=<n>&value=<text>`, protected by the same `X-Requested-With` header as the other write endpoints. `/data/devices.json` now has a `location` field.
- **Alarm Control Units.**
  - The Docked, Sensor battery and Radio module battery entities are still created but are **disabled by default** in Home Assistant, because the control units do not report them. Enable them in Home Assistant if you want them.
  - The web interface shows **N/A** for Docked and Faults on control units. An error is still shown if one is reported.
- **Shorter entity names.** Entities were named with the model and device ID twice (for example `binary_sensor.fireangel_unknown_5096591_unknown_5096591_battery`) because Home Assistant adds the device name in front of the entity name. Entity names are now just "Smoke detector", "Docked", "Sensor battery" and so on.
- **Upgrade notes**
  - Existing entities keep their old IDs and names in Home Assistant. To get the new names, use **Forget all devices** in the web interface, delete the old devices and entities in Home Assistant, and let the gateway rediscover them. Re-enter the locations if the database was reset.
  - Entity IDs will change, so update any automation or dashboard that uses them.

### V1.4.7
- **Click feedback on the Probe buttons.** **Probe** and **Probe all devices** turn green and say "Sent" for about 1.5 seconds after they are clicked, then go back to normal. It stays green when the table refreshes in between, and if the request fails the button goes back to normal straight away and the existing error message is shown.
- **Forgetting a single device now clears its location.** **Forget all devices** still keeps the locations, so devices that come straight back have theirs. The confirmation messages say which applies. (In V1.4.6 both kept the location.)

### V1.5.0
- **Show unidentified SIDs (web interface).** A new **Show unidentified SIDs** checkbox, in the same style as the options in the MQTT messages list, lists SIDs that are on the radio network but have never been identified (for example SID 8). For each one it shows how many identity requests were sent, how many got no answer and when it was last asked. The count is always shown beside the checkbox. `/data/devices.json` has a new `unidentified` list.
- **Unrecognised packet counter.** Packets the gateway does not know how to handle are now counted instead of silently ignored, by type (and sub type for `0xD4` packets, up to 8 types) with the first bytes of the latest one. They appear in the Radio module card, in `/data/module.json` (`unrecognised`), in the serial log, and as two new bridge sensors in Home Assistant (`wisafe_rx_unrecognised`, `wisafe_rx_unrecognised_last`). A truncated `0xD4` packet is counted too.
- **"None reported" instead of "Unknown".** The Alarm, Docked and Faults pills say "None reported" when a device has not reported a value.
- **Everything on the web page is now in Home Assistant.**
  - Each device gets a **Probe** button and diagnostic sensors: Signal (RSSI) and radio faults (enabled), and SID, battery 1, battery 2, firmware, flags, status sent / replies / no answer and the raw status reply (disabled by default, enable the ones you want). They update when a status reply arrives or a query is sent or times out.
  - The bridge gets **Probe All Devices** (enabled) and **Rescan Radio Network** (disabled by default) buttons.
  - Forget is deliberately not available in Home Assistant, because it is destructive.
  - Forgetting a device removes the new entities as well.
- **MQTT command hardening.** `probe_<id>` only accepts plain digits.
- **Upgrade notes.** The new Home Assistant entities are created automatically when devices are next announced (restart the gateway, or Forget all devices). Not tested on real hardware yet.

### V1.5.1
- **Web interface works behind a reverse proxy.** The page used absolute addresses (`/data/system.json`, `/api/probe` and so on), so behind a proxy that serves it under a path prefix, such as Home Assistant ingress (`/api/hassio_ingress/<token>/`), the page loaded but every request went to the proxy's own root and no data appeared. All page requests and the icon are now relative (`data/system.json`, `api/probe`, `favicon.svg`), so the page works from any prefix. The gateway's own endpoints are unchanged. The page must be opened with a trailing slash (`.../<token>/`), which ingress does.
- **Removed the unconfirmed firmware value from the device rows.** The "fw?" figure under RSSI is gone, because the value is identical on every device and its meaning is unknown. The Radio module card still shows Module firmware (raw), and the value is still in `/data/*.json` and in the (disabled by default) Home Assistant Firmware sensor.
- **Mark all docked.** A **Mark all docked** button next to Forget all devices (with a confirmation) records every alarm as docked, because the docked state cannot be read from a device and a reset leaves it as "None reported". Control units are skipped. It updates the web page, the database and Home Assistant, leaves the other fault flags as they were, and does not change "last heard". The next real report from a device replaces it. New endpoint `POST /api/mark-docked` (same `X-Requested-With` protection) and a bridge button **Mark All Devices Docked** in Home Assistant (disabled by default).
- **Radio module responding.** The gateway polls the module every minute, so if nothing at all has arrived from the module for 10 minutes it is treated as not answering. This is shown as **Module responding** in the Radio module card (`rx.responding` in `/data/module.json`) and as a new bridge binary sensor **Radio Module Responding** in Home Assistant (connectivity, on/off, sent on change and as a heartbeat). The clock starts at boot, so a restart does not raise a false alarm for 10 minutes. Use it for an alert automation.

### V1.5.2
- **Battery column.** The device table has a new **Battery** column: OK, Sensor battery low, Radio battery low, None reported, or N/A for control units. The Faults column now shows only Error (or None). Battery is a low-battery flag, not a level.
- **Mark all battery OK.** A **Mark all battery OK** button (with a confirmation) records good batteries for devices that have not reported, for the same reason as Mark all docked. **A device that shows any fault (an error or either battery flag) is never changed**, and the page says how many were left alone. Control units are skipped. New endpoint `POST /api/mark-battery-ok` and a bridge button **Mark All Batteries OK** in Home Assistant (disabled by default).
- **Docked and battery are now tracked separately.** Marking one no longer claims the other. A device marked battery OK still shows Docked as "None reported" (and the other way round), and Home Assistant only gets the values that are known. Stored in the existing database columns (the value 2 means not known), so there is no schema change and existing data is read as before. Because the state is shared, a device that has only been marked still shows Faults as "None".
- **Mark all docked no longer overrides a device that has reported it is not docked.** Such devices are left as they are and the page says how many were skipped.

