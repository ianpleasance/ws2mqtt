#include "Arduino.h"
#include "device_runtime.h"

static device_runtime_t devices[RUNTIME_MAX_SID];

static device_runtime_t* slotForSid(uint8_t sid) {
	if (sid >= RUNTIME_MAX_SID)
		return NULL;

	device_runtime_t* d = &devices[sid];
	if (!d->known) {
		memset(d, 0, sizeof(*d));
		d->known = true;
		d->sid = sid;
	}
	return d;
}

device_runtime_t* runtimeBySid(uint8_t sid) {
	if (sid >= RUNTIME_MAX_SID || !devices[sid].known)
		return NULL;
	return &devices[sid];
}

device_runtime_t* runtimeByDeviceId(uint32_t device_id) {
	for (int i = 0; i < RUNTIME_MAX_SID; i++)
		if (devices[i].known && devices[i].device_id == device_id)
			return &devices[i];
	return NULL;
}

void runtimeClear() {
	memset(devices, 0, sizeof(devices));
}

void runtimeSetInfo(const device_t& device) {
	device_runtime_t* d = slotForSid(device.sid);
	if (d == NULL)
		return;

	d->device_id = device.device_id;
	d->model = device.model;
	d->device_type = device.device_type;
	d->lastSeenMs = millis();
}

void runtimeSetState(const device_state_t& ds) {
	device_runtime_t* d = slotForSid(ds.sid);
	if (d == NULL)
		return;

	d->device_id = ds.device_id;
	d->haveState = true;
	d->generic_error = ds.generic_error;
	d->docked = ds.docked;
	d->sensor_battery = ds.sensor_battery;
	d->radio_module_battery = ds.radio_module_battery;
	d->lastSeenMs = millis();
}

void runtimeSetAlarm(uint32_t device_id, bool alarm) {
	device_runtime_t* d = runtimeByDeviceId(device_id);
	if (d == NULL)
		return;

	d->alarmKnown = true;
	d->alarm = alarm;
	d->lastSeenMs = millis();
	if (alarm)
		d->lastAlarmMs = millis();
}

void runtimeSetButton(uint32_t device_id) {
	device_runtime_t* d = runtimeByDeviceId(device_id);
	if (d == NULL)
		return;

	d->lastButtonMs = millis();
	d->lastSeenMs = millis();
}

void runtimeSetDiag(uint8_t sid, uint32_t device_id, uint8_t battery1, uint8_t battery2,
	uint8_t rssi, uint8_t firmware_version, uint8_t flags, uint8_t radio_fault_count) {

	device_runtime_t* d = slotForSid(sid);
	if (d == NULL)
		return;

	d->device_id = device_id;
	d->haveDiag = true;
	d->battery1 = battery1;
	d->battery2 = battery2;
	d->rssi = rssi;
	d->firmware_version = firmware_version;
	d->flags = flags;
	d->radio_fault_count = radio_fault_count;
	d->lastSeenMs = millis();
}

static void loadDevice(device_t device) {
	// Devices loaded from the database have not been heard from since boot,
	// so leave lastSeenMs at zero
	device_runtime_t* d = slotForSid(device.sid);
	if (d == NULL)
		return;

	d->device_id = device.device_id;
	d->model = device.model;
	d->device_type = device.device_type;
}

static void loadDeviceState(device_state_t ds) {
	device_runtime_t* d = slotForSid(ds.sid);
	if (d == NULL)
		return;

	d->device_id = ds.device_id;
	d->haveState = true;
	d->generic_error = ds.generic_error;
	d->docked = ds.docked;
	d->sensor_battery = ds.sensor_battery;
	d->radio_module_battery = ds.radio_module_battery;
}

void runtimeLoadFromDB() {
	runtimeClear();
	getAllDevicesFromDB(loadDevice);
	getAllDeviceStatesFromDB(loadDeviceState);
}
