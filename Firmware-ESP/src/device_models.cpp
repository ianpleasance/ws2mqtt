#include "Arduino.h"
#include "device_models.h"
#include "wisafe2_packets.h"

// ---------------------------------------------------------------------------
// Device types
// ---------------------------------------------------------------------------
static const device_type_info_t typeTable[] = {
	{ DEVICE_TYPE_CO,           "CO",           "carbon_monoxide", "CO detector"    },
	{ DEVICE_TYPE_SMOKE,        "Smoke",        "smoke",           "Smoke detector" },
	{ DEVICE_TYPE_HEAT,         "Heat",         "heat",            "Heat detector"  },
	// Alarm Control Units never alarm themselves; they keep the generic
	// "safety" class so the entity layout matches earlier versions
	{ DEVICE_TYPE_CONTROL_UNIT, "Control unit", "safety",          "Alarm"          },
};

static const device_type_info_t typeUnset   = { 0x00, "Unknown", "safety", "Detector" };
static const device_type_info_t typeUnknown = { 0xFF, "Other",   "safety", "Detector" };

const device_type_info_t* deviceTypeInfo(uint8_t type) {
	if (type == 0x00)
		return &typeUnset;

	for (size_t i = 0; i < sizeof(typeTable) / sizeof(typeTable[0]); i++)
		if (typeTable[i].type == type)
			return &typeTable[i];

	return &typeUnknown;
}

// ---------------------------------------------------------------------------
// Models
// ---------------------------------------------------------------------------
typedef struct {
	uint16_t model;
	const char* name;
} model_info_t;

static const model_info_t modelTable[] = {
	{ MODEL_WST630,             "WST630"            },
	{ MODEL_W2CO10X,            "W2-CO-10X"         },
	{ MODEL_FP1720W2R,          "FP1720W2-R"        },
	{ MODEL_ST630DE,            "ST-630-DE"         },
	{ MODEL_W2SVP630,           "W2-SVP-630"        },
	{ MODEL_FP2620W2,           "FP2620W2"          },
	{ MODEL_FP2620W2R,          "FP2620W2-R"        },
	{ MODEL_ALARM_CONTROL_UNIT, "Alarm Control Unit" },
};

bool modelIsKnown(uint16_t model_id) {
	for (size_t i = 0; i < sizeof(modelTable) / sizeof(modelTable[0]); i++)
		if (modelTable[i].model == model_id)
			return true;
	return false;
}

const char* modelString(uint16_t model_id) {
	static char unknown[20];

	for (size_t i = 0; i < sizeof(modelTable) / sizeof(modelTable[0]); i++)
		if (modelTable[i].model == model_id)
			return modelTable[i].name;

	snprintf(unknown, sizeof(unknown), "Unknown 0x%04X", model_id);
	return unknown;
}
