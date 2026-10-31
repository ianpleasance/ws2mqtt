#include "Arduino.h"
#include "device_manage.h"
#include "device_db.h"
#include "device_runtime.h"
#include "homeassistant.h"
#include "status_query.h"
#include "wisafe2_tx.h"

// Remove a device everywhere. The saved location is only cleared when
// keepLocation is false.
static bool forgetOne(uint8_t sid, bool keepLocation) {
	device_t device;
	memset(&device, 0, sizeof(device));

	const device_runtime_t* rt = runtimeBySid(sid);
	if (rt != NULL) {
		device.sid = rt->sid;
		device.device_id = rt->device_id;
		device.model = rt->model;
		device.device_type = rt->device_type;
	} else {
		getDeviceFromDB(sid, &device);
		if (device.device_type == 0x00)
			return false;
	}

	Serial.printf("Forgetting SID %d, device_id %u\n", sid, (unsigned int)device.device_id);

	removeMQTTDevice(device);
	deleteDeviceDB(sid);
	if (!keepLocation)
		setLocationDB(device.device_id, "");
	runtimeForget(sid);
	queryForgetSid(sid);
	return true;
}

bool forgetDevice(uint8_t sid) {
	return forgetOne(sid, false);
}

int forgetAllDevices() {
	// Locations are kept, so devices that come straight back have theirs
	int count = 0;
	for (int sid = 0; sid < RUNTIME_MAX_SID; sid++)
		if (runtimeBySid(sid) != NULL && forgetOne(sid, true))
			count++;

	// Rescan right away so devices that are still on the network come back
	sendQuerySIDMap();
	return count;
}

bool probeDevice(uint8_t sid) {
	if (runtimeBySid(sid) == NULL)
		return false;
	Serial.printf("Probing SID %d\n", sid);
	queryRequestFirst(sid, SPI_TX_DIAGNOSTIC_DETAILS_STATUS);
	return true;
}

int probeAllDevices() {
	int count = 0;
	for (int sid = 0; sid < RUNTIME_MAX_SID; sid++) {
		if (runtimeBySid(sid) == NULL)
			continue;
		queryRequest(sid, SPI_TX_DIAGNOSTIC_DETAILS_STATUS);
		count++;
	}
	return count;
}

// Remove control characters, trim spaces and cut to the maximum length without
// splitting a UTF-8 character
static void cleanLocation(const char* in, char* out, size_t outSize) {
	size_t n = 0;
	bool started = false;
	for (const unsigned char* p = (const unsigned char*)(in != NULL ? in : ""); *p != '\0'; p++) {
		if (*p < 0x20 || *p == 0x7F)
			continue;
		if (*p == ' ' && !started)
			continue;
		started = true;
		if (n + 1 >= outSize)
			break;
		out[n++] = (char)*p;
	}
	while (n > 0 && out[n - 1] == ' ')
		n--;
	// Do not leave half of a multi-byte character at the end
	if (n > 0 && ((unsigned char)out[n - 1] & 0x80) != 0) {
		size_t i = n;
		while (i > 0 && ((unsigned char)out[i - 1] & 0xC0) == 0x80)
			i--;
		if (i > 0) {
			unsigned char lead = (unsigned char)out[i - 1];
			size_t need = (lead >= 0xF0) ? 4 : (lead >= 0xE0) ? 3 : (lead >= 0xC0) ? 2 : 1;
			if (n - (i - 1) < need)
				n = i - 1;
		}
	}
	out[n] = '\0';
}

bool setDeviceLocation(uint8_t sid, const char* value) {
	device_runtime_t* rt = runtimeBySid(sid);
	if (rt == NULL)
		return false;

	char clean[LOCATION_MAX_LEN + 1];
	cleanLocation(value, clean, sizeof(clean));

	snprintf(rt->location, sizeof(rt->location), "%s", clean);
	setLocationDB(rt->device_id, clean);
	sendMQTTLocation(rt->device_id, clean);
	Serial.printf("Location of SID %d set to \"%s\"\n", sid, clean);
	return true;
}
