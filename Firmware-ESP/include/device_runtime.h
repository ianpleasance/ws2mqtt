#ifndef DEVICE_RUNTIME_H
#define DEVICE_RUNTIME_H 1

#include <stdint.h>
#include "device_db.h"

// SIDs on a WiSafe2 network are 0..63
#define RUNTIME_MAX_SID 64

// In-memory view of each paired device, kept for the web UI. The SQLite
// database stores what must survive a reboot (identity and error flags); this
// adds what is only known at runtime such as the alarm state and when each
// device was last heard from.
typedef struct {
	bool known;            // slot is in use
	uint8_t sid;
	uint32_t device_id;
	uint16_t model;
	uint8_t device_type;

	// Alarm state. alarmKnown is false until we have seen an alarm / alarm off
	// event or the initial status response, since the module cannot be asked.
	bool alarmKnown;
	bool alarm;

	// Error flags from the last error packet (or the database at boot)
	bool haveState;
	bool generic_error;
	bool docked;
	bool sensor_battery;
	bool radio_module_battery;

	// Last remote status response (D4 06 .. 00)
	bool haveDiag;
	uint8_t battery1;
	uint8_t battery2;
	uint8_t rssi;
	uint8_t firmware_version;
	uint8_t flags;
	uint8_t radio_fault_count;

	// millis() timestamps, 0 = never
	uint32_t lastSeenMs;
	uint32_t lastAlarmMs;
	uint32_t lastButtonMs;
} device_runtime_t;

// Load devices and error flags from the database. Call after setupDeviceDB().
void runtimeLoadFromDB();

// Forget every device (module was unpaired and the database reset)
void runtimeClear();

// Look up a device. Returns NULL if unknown.
device_runtime_t* runtimeBySid(uint8_t sid);
device_runtime_t* runtimeByDeviceId(uint32_t device_id);

void runtimeSetInfo(const device_t& device);
void runtimeSetState(const device_state_t& ds);
void runtimeSetAlarm(uint32_t device_id, bool alarm);
void runtimeSetButton(uint32_t device_id);
void runtimeSetDiag(uint8_t sid, uint32_t device_id, uint8_t battery1, uint8_t battery2,
	uint8_t rssi, uint8_t firmware_version, uint8_t flags, uint8_t radio_fault_count);

#endif // DEVICE_RUNTIME_H
