#include "Arduino.h"
#include "device_sensors.h"

typedef bool (*value_fn)(const device_runtime_t* d, char* out, size_t n);

typedef struct {
	device_sensor_t info;
	value_fn value;
} entry_t;

static bool putUInt(char* out, size_t n, unsigned long v) {
	snprintf(out, n, "%lu", v);
	return true;
}

// Values from the last status reply exist only after the device has answered
static bool vSid(const device_runtime_t* d, char* o, size_t n)        { return putUInt(o, n, d->sid); }
static bool vRssi(const device_runtime_t* d, char* o, size_t n)       { return d->haveDiag && putUInt(o, n, d->rssi); }
static bool vBattery1(const device_runtime_t* d, char* o, size_t n)   { return d->haveDiag && putUInt(o, n, d->battery1); }
static bool vBattery2(const device_runtime_t* d, char* o, size_t n)   { return d->haveDiag && putUInt(o, n, d->battery2); }
static bool vFirmware(const device_runtime_t* d, char* o, size_t n)   { return d->haveDiag && putUInt(o, n, d->firmware_version); }
static bool vFaults(const device_runtime_t* d, char* o, size_t n)     { return d->haveDiag && putUInt(o, n, d->radio_fault_count); }
static bool vFlags(const device_runtime_t* d, char* o, size_t n) {
	if (!d->haveDiag)
		return false;
	snprintf(o, n, "0x%02X", (unsigned)d->flags);
	return true;
}
static bool vSent(const device_runtime_t* d, char* o, size_t n)       { return putUInt(o, n, d->statusQueries); }
static bool vReplies(const device_runtime_t* d, char* o, size_t n)    { return putUInt(o, n, d->statusReplies); }
static bool vNoAnswer(const device_runtime_t* d, char* o, size_t n)   { return putUInt(o, n, d->statusNoAnswer); }
static bool vRaw(const device_runtime_t* d, char* o, size_t n) {
	if (d->statusRawLen == 0)
		return false;
	size_t len = 0;
	o[0] = 0;
	for (int i = 0; i < d->statusRawLen && len + 4 < n; i++)
		len += snprintf(o + len, n - len, i ? " %02X" : "%02X", d->statusRaw[i]);
	return true;
}

// Signal and radio faults are on by default; the rest can be switched on in
// Home Assistant when wanted.
static const entry_t entries[] = {
	{{"rssi",           "Signal (RSSI)",        "None", "None", "mdi:signal",              true},  vRssi},
	{{"radio_faults",   "Radio faults",         "None", "None", "mdi:alert-circle",        true},  vFaults},
	{{"sid",            "SID",                  "None", "None", "mdi:identifier",          false}, vSid},
	{{"battery1",       "Battery 1 (raw)",      "None", "None", "mdi:battery",             false}, vBattery1},
	{{"battery2",       "Battery 2 (raw)",      "None", "None", "mdi:battery",             false}, vBattery2},
	{{"firmware",       "Firmware (raw)",       "None", "None", "mdi:tag",                 false}, vFirmware},
	{{"flags",          "Flags",                "None", "None", "mdi:flag",                false}, vFlags},
	{{"status_sent",    "Status queries sent",  "None", "None", "mdi:counter",             false}, vSent},
	{{"status_replies", "Status replies",       "None", "None", "mdi:counter",             false}, vReplies},
	{{"status_no_answer","Status no answer",    "None", "None", "mdi:counter",             false}, vNoAnswer},
	{{"status_raw",     "Status reply (raw)",   "None", "None", "mdi:code-braces",         false}, vRaw},
};

int deviceSensorCount() {
	return (int)(sizeof(entries) / sizeof(entries[0]));
}

const device_sensor_t* deviceSensor(int index) {
	if (index < 0 || index >= deviceSensorCount())
		return NULL;
	return &entries[index].info;
}

bool deviceSensorValue(int index, const device_runtime_t* d, char* out, size_t outSize) {
	if (index < 0 || index >= deviceSensorCount() || d == NULL || outSize == 0)
		return false;
	out[0] = 0;
	return entries[index].value(d, out, outSize);
}

int deviceSensorIndex(const char* key) {
	for (int i = 0; i < deviceSensorCount(); i++)
		if (strcmp(entries[i].info.key, key) == 0)
			return i;
	return -1;
}
