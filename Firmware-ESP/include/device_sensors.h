#ifndef DEVICE_SENSORS_H
#define DEVICE_SENSORS_H 1

#include <stddef.h>
#include "device_runtime.h"

// Table of the per-device values shown in the web interface that are also sent
// to MQTT as diagnostic sensors on each device in Home Assistant.
typedef struct {
	const char* key;       // topic suffix and unique_id suffix (never change an existing key)
	const char* name;      // Home Assistant entity name (short; the device name is added in front)
	const char* devClass;  // "None" for no device class
	const char* unit;      // "None" for no unit
	const char* icon;
	bool enabled;          // enabled by default in Home Assistant
} device_sensor_t;

int deviceSensorCount();
const device_sensor_t* deviceSensor(int index);

// Writes the current value as text. Returns false when the device has no such
// value yet (nothing is published in that case).
bool deviceSensorValue(int index, const device_runtime_t* d, char* out, size_t outSize);

// Index of the sensor with this key, or -1
int deviceSensorIndex(const char* key);

#endif // DEVICE_SENSORS_H
