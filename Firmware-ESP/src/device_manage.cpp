#include "Arduino.h"
#include "device_manage.h"
#include "device_db.h"
#include "device_runtime.h"
#include "homeassistant.h"
#include "status_query.h"
#include "wisafe2_tx.h"

bool forgetDevice(uint8_t sid) {
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
	runtimeForget(sid);
	queryForgetSid(sid);
	return true;
}

int forgetAllDevices() {
	int count = 0;
	for (int sid = 0; sid < RUNTIME_MAX_SID; sid++)
		if (runtimeBySid(sid) != NULL && forgetDevice(sid))
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
