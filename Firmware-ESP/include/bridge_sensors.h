#ifndef BRIDGE_SENSORS_H
#define BRIDGE_SENSORS_H 1

#include <stddef.h>

// Table of every ESP32, WiSafe module and network/MQTT value that is sent to
// MQTT and shown as a sensor on the bridge device in Home Assistant.
typedef struct {
	const char* key;       // topic suffix and unique_id suffix (never change an existing key)
	const char* name;      // Home Assistant entity name
	const char* devClass;  // "None" for no device class
	const char* unit;      // "None" for no unit
	const char* icon;
	bool enabled;          // enabled by default in Home Assistant
} bridge_sensor_t;

int bridgeSensorCount();
const bridge_sensor_t* bridgeSensor(int index);

// Writes the current value as text. Returns false when there is no value yet
// (nothing is published in that case).
bool bridgeSensorValue(int index, char* out, size_t outSize);

#endif // BRIDGE_SENSORS_H
