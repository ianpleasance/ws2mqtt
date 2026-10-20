#ifndef DEVICE_MODELS_H
#define DEVICE_MODELS_H 1

#include <stdint.h>

// What we know about a kind of WiSafe2 device, identified by the device type
// byte in its identity packet.
typedef struct {
	uint8_t type;            // device type code, e.g. 0x81
	const char* name;        // short name shown in the web UI, e.g. "Smoke"
	const char* haClass;     // Home Assistant binary_sensor device class
	const char* entityName;  // name of the alarm entity in Home Assistant
} device_type_info_t;

// Information for a device type. Never returns NULL: unrecognised types get a
// generic entry so they still work and show up.
const device_type_info_t* deviceTypeInfo(uint8_t type);

// Product name for a model code, e.g. 0x08ED -> "FP2620W2". Unrecognised codes
// come back as "Unknown 0xNNNN" so they identify themselves. The result is only
// valid until the next call.
const char* modelString(uint16_t model_id);

// True if the model code is one we have a product name for
bool modelIsKnown(uint16_t model_id);

#endif // DEVICE_MODELS_H
