#ifndef WEB_DATA_H
#define WEB_DATA_H 1

#include "Arduino.h"

// JSON writers for the gateway's internal data. They stream to a Print so no
// large buffer is needed. The web UI uses them now; the token protected API
// reuses the same writers.
void writeSystemJson(Print& out);    // ESP32, network and MQTT connection
void writeModuleJson(Print& out);    // WiSafe2 radio module and UART statistics
void writeDevicesJson(Print& out);   // Paired WiSafe2 devices and their states
void writeMqttJson(Print& out, bool includeHousekeeping);  // Recent MQTT messages

#endif // WEB_DATA_H
