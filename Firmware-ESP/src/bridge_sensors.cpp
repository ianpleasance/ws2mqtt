#include "Arduino.h"
#include <WiFi.h>
#include <PubSubClient.h>

#include "bridge_sensors.h"
#include "config.h"
#include "credentials.h"
#include "version.h"
#include "sysinfo.h"
#include "module_info.h"
#include "wisafe2_tx.h"

extern PubSubClient mqttClient;
extern int ownSid;

typedef bool (*value_fn)(char* out, size_t n);

typedef struct {
	bridge_sensor_t info;
	value_fn value;
} entry_t;

static bool putStr(char* out, size_t n, const char* s) {
	snprintf(out, n, "%s", s != NULL ? s : "");
	return true;
}
static bool putUInt(char* out, size_t n, unsigned long v) {
	snprintf(out, n, "%lu", v);
	return true;
}
static bool putInt(char* out, size_t n, long v) {
	snprintf(out, n, "%ld", v);
	return true;
}

// ---- ESP32 --------------------------------------------------------------
static bool vIp(char* o, size_t n)        { return putStr(o, n, WiFi.localIP().toString().c_str()); }
static bool vRssi(char* o, size_t n)      { return putInt(o, n, WiFi.RSSI()); }
static bool vMac(char* o, size_t n)       { return putStr(o, n, WiFi.macAddress().c_str()); }
static bool vSsid(char* o, size_t n)      { return putStr(o, n, WiFi.SSID().c_str()); }
static bool vUptime(char* o, size_t n)    { return putUInt(o, n, uptimeSeconds()); }
static bool vReset(char* o, size_t n)     { return putStr(o, n, resetReasonString(esp_reset_reason())); }
static bool vFirmware(char* o, size_t n)  { return putStr(o, n, FIRMWARE_VERSION); }
static bool vBuild(char* o, size_t n)     { return putStr(o, n, buildTimestamp()); }
static bool vModel(char* o, size_t n)     { return putStr(o, n, ESP.getChipModel()); }
static bool vChipRev(char* o, size_t n)   { return putInt(o, n, ESP.getChipRevision()); }
static bool vCores(char* o, size_t n)     { return putInt(o, n, ESP.getChipCores()); }
static bool vCpu(char* o, size_t n)       { return putInt(o, n, ESP.getCpuFreqMHz()); }
static bool vFlash(char* o, size_t n)     { return putUInt(o, n, ESP.getFlashChipSize()); }
static bool vSketch(char* o, size_t n)    { return putUInt(o, n, ESP.getSketchSize()); }
static bool vHeapFree(char* o, size_t n)  { return putUInt(o, n, ESP.getFreeHeap()); }
static bool vHeapTotal(char* o, size_t n) { return putUInt(o, n, ESP.getHeapSize()); }
static bool vHeapMin(char* o, size_t n)   { return putUInt(o, n, ESP.getMinFreeHeap()); }
static bool vSdk(char* o, size_t n)       { return putStr(o, n, ESP.getSdkVersion()); }
static bool vBridgeName(char* o, size_t n){ snprintf(o, n, "ws2mqtt_%08x", DEVICE_ID); return true; }
static bool vBridgeId(char* o, size_t n)  { snprintf(o, n, "0x%06X", (unsigned int)DEVICE_ID); return true; }

// ---- Network and MQTT ---------------------------------------------------
static bool vHostname(char* o, size_t n)  { return putStr(o, n, WiFi.getHostname()); }
static bool vBssid(char* o, size_t n)     { return putStr(o, n, WiFi.BSSIDstr().c_str()); }
static bool vChannel(char* o, size_t n)   { return putInt(o, n, WiFi.channel()); }
static bool vMqttHost(char* o, size_t n)  { return putStr(o, n, MQTT_HOST); }
static bool vMqttConnects(char* o, size_t n)  { return putUInt(o, n, mqttStats.connects); }
static bool vMqttAttempts(char* o, size_t n)  { return putUInt(o, n, mqttStats.connectAttempts); }
static bool vMqttSent(char* o, size_t n)      { return putUInt(o, n, mqttStats.published); }
static bool vMqttFailed(char* o, size_t n)    { return putUInt(o, n, mqttStats.publishFailed); }
static bool vMqttReceived(char* o, size_t n)  { return putUInt(o, n, mqttStats.received); }
static bool vMqttLastError(char* o, size_t n) {
	if (mqttStats.connectAttempts > mqttStats.connects)
		return putStr(o, n, mqttStateString(mqttStats.lastFailState));
	return putStr(o, n, "None");
}

// ---- WiSafe module ------------------------------------------------------
static bool vPaired(char* o, size_t n) {
	if (ownSid < 0)
		return false;
	return putStr(o, n, ownSid != 0x40 ? "Yes" : "No");
}
static bool vModuleSid(char* o, size_t n) {
	if (ownSid < 0)
		return false;
	return putInt(o, n, ownSid);
}
static bool vGatewayId(char* o, size_t n) { return vBridgeId(o, n); }
static bool vModuleDevId(char* o, size_t n) {
	if (!moduleInfo.diagReceived)
		return false;
	return putUInt(o, n, moduleInfo.device_id);
}
static bool vModuleFw(char* o, size_t n) {
	if (!moduleInfo.diagReceived)
		return false;
	return putInt(o, n, moduleInfo.firmware_version);
}
static bool vModuleBattery(char* o, size_t n) {
	if (!moduleInfo.diagReceived)
		return false;
	snprintf(o, n, "%u / %u", (unsigned)moduleInfo.battery1, (unsigned)moduleInfo.battery2);
	return true;
}
static bool vModuleRssi(char* o, size_t n) {
	if (!moduleInfo.diagReceived)
		return false;
	return putInt(o, n, moduleInfo.rssi);
}
static bool vModuleFlags(char* o, size_t n) {
	if (!moduleInfo.diagReceived)
		return false;
	snprintf(o, n, "0x%02X", (unsigned)moduleInfo.flags);
	return true;
}
static bool vModuleFaults(char* o, size_t n) {
	if (!moduleInfo.diagReceived)
		return false;
	return putInt(o, n, moduleInfo.radio_fault_count);
}
static bool vSids(char* o, size_t n) {
	if (!moduleInfo.sidmapReceived)
		return false;
	size_t len = 0;
	o[0] = 0;
	for (int sid = 0; sid < 64; sid++) {
		if ((moduleInfo.sidmap >> sid) & 0x01) {
			int w = snprintf(o + len, n - len, len ? ", %d" : "%d", sid);
			if (w < 0 || (size_t)w >= n - len)
				break;
			len += w;
		}
	}
	return true;
}
static bool vSidMap(char* o, size_t n) {
	if (!moduleInfo.sidmapReceived)
		return false;
	snprintf(o, n, "%08lx%08lx",
		(unsigned long)(moduleInfo.sidmap >> 32), (unsigned long)(moduleInfo.sidmap & 0xFFFFFFFFUL));
	return true;
}
static bool vWelcomes(char* o, size_t n)    { return putUInt(o, n, moduleInfo.welcomeCount); }
static bool vRxFrames(char* o, size_t n)    { return putUInt(o, n, moduleInfo.rxFrames); }
static bool vRxTimeouts(char* o, size_t n)  { return putUInt(o, n, moduleInfo.rxFrameTimeouts); }
static bool vRxBadLen(char* o, size_t n)    { return putUInt(o, n, moduleInfo.rxBadLength); }
static bool vRxShort(char* o, size_t n)     { return putUInt(o, n, moduleInfo.rxShortPackets); }
static bool vTxSent(char* o, size_t n)      { return putUInt(o, n, moduleInfo.txSent); }
static bool vTxDropped(char* o, size_t n)   { return putUInt(o, n, moduleInfo.txDropped); }
static bool vTxQueue(char* o, size_t n)     { return putInt(o, n, txQueueDepth()); }

// The first group keeps the keys, names and enabled flags the sensors already had
static const entry_t entries[] = {
	// ESP32
	{{"wifi_ip_addr",          "IP Address",            "None",      "None", "mdi:ip-network",     true},  vIp},
	{{"wifi_rssi",             "RSSI",                  "signal_strength", "dBm", "mdi:wifi",      true},  vRssi},
	{{"wifi_mac_addr",         "MAC Address",           "None",      "None", "mdi:wifi",           true},  vMac},
	{{"wifi_ssid",             "SSID",                  "None",      "None", "mdi:wifi",           true},  vSsid},
	{{"esp_uptime",       "ESP Uptime",            "duration",  "s",    "mdi:clock",          false}, vUptime},
	{{"esp_reset_reason", "ESP Restart Reason",    "None",      "None", "mdi:restart-alert",  false}, vReset},
	{{"firmware_version", "Firmware Version",      "None",      "None", "mdi:tag",            true},  vFirmware},
	{{"esp_model",        "ESP Model",             "None",      "None", "mdi:chip",           false}, vModel},

	{{"firmware_build",   "Firmware Build",        "None",      "None", "mdi:calendar-clock", true},  vBuild},
	{{"esp_chip_revision","ESP Chip Revision",     "None",      "None", "mdi:chip",           true},  vChipRev},
	{{"esp_cores",        "ESP Cores",             "None",      "None", "mdi:chip",           true},  vCores},
	{{"esp_cpu_mhz",      "ESP CPU Speed",         "frequency", "MHz",  "mdi:speedometer",    true},  vCpu},
	{{"esp_flash_size",   "ESP Flash Size",        "data_size", "B",    "mdi:memory",         true},  vFlash},
	{{"esp_firmware_size","ESP Firmware Size",     "data_size", "B",    "mdi:memory",         true},  vSketch},
	{{"esp_heap_free",    "ESP Heap Free",         "data_size", "B",    "mdi:memory",         true},  vHeapFree},
	{{"esp_heap_total",   "ESP Heap Total",        "data_size", "B",    "mdi:memory",         true},  vHeapTotal},
	{{"esp_heap_min_free","ESP Heap Lowest Free",  "data_size", "B",    "mdi:memory",         true},  vHeapMin},
	{{"esp_sdk_version",  "ESP SDK Version",       "None",      "None", "mdi:tag",            true},  vSdk},
	{{"bridge_name",      "Bridge Name",           "None",      "None", "mdi:identifier",     true},  vBridgeName},
	{{"bridge_id",        "Bridge ID",             "None",      "None", "mdi:identifier",     true},  vBridgeId},

	// Network and MQTT
	{{"wifi_hostname",    "Hostname",              "None",      "None", "mdi:wifi",           true},  vHostname},
	{{"wifi_bssid",       "BSSID",                 "None",      "None", "mdi:wifi",           true},  vBssid},
	{{"wifi_channel",     "WiFi Channel",          "None",      "None", "mdi:wifi",           true},  vChannel},
	{{"mqtt_broker",      "MQTT Broker",           "None",      "None", "mdi:server-network", true},  vMqttHost},
	{{"mqtt_connects",    "MQTT Connects",         "None",      "None", "mdi:counter",        true},  vMqttConnects},
	{{"mqtt_attempts",    "MQTT Connect Attempts", "None",      "None", "mdi:counter",        true},  vMqttAttempts},
	{{"mqtt_last_error",  "MQTT Last Connect Error","None",     "None", "mdi:alert-circle",   true},  vMqttLastError},
	{{"mqtt_sent",        "MQTT Messages Sent",    "None",      "None", "mdi:counter",        true},  vMqttSent},
	{{"mqtt_failed",      "MQTT Messages Failed",  "None",      "None", "mdi:counter",        true},  vMqttFailed},
	{{"mqtt_received",    "MQTT Messages Received","None",      "None", "mdi:counter",        true},  vMqttReceived},

	// WiSafe module
	{{"wisafe_paired",        "WiSafe Paired",          "None", "None", "mdi:link",          true},  vPaired},
	{{"wisafe_module_sid",    "WiSafe Module SID",      "None", "None", "mdi:identifier",    true},  vModuleSid},
	{{"wisafe_gateway_id",    "WiSafe Gateway ID",      "None", "None", "mdi:identifier",    true},  vGatewayId},
	{{"wisafe_module_id",     "WiSafe Module Device ID","None", "None", "mdi:identifier",    true},  vModuleDevId},
	{{"wisafe_module_fw",     "WiSafe Module Firmware (raw)", "None", "None", "mdi:tag",     true},  vModuleFw},
	{{"wisafe_module_battery","WiSafe Module Battery (raw)",  "None", "None", "mdi:battery", true},  vModuleBattery},
	{{"wisafe_module_rssi",   "WiSafe Module RSSI (raw)",     "None", "None", "mdi:signal",  true},  vModuleRssi},
	{{"wisafe_module_flags",  "WiSafe Module Flags",    "None", "None", "mdi:flag",          true},  vModuleFlags},
	{{"wisafe_radio_faults",  "WiSafe Radio Faults",    "None", "None", "mdi:alert-circle",  true},  vModuleFaults},
	{{"wisafe_sids",          "WiSafe SIDs On Network", "None", "None", "mdi:format-list-numbered", true}, vSids},
	{{"wisafe_sid_map",       "WiSafe SID Map",         "None", "None", "mdi:format-list-numbered", true}, vSidMap},
	{{"wisafe_welcomes",      "WiSafe Welcome Handshakes","None","None","mdi:handshake",     true},  vWelcomes},
	{{"wisafe_rx_frames",     "WiSafe Frames Received", "None", "None", "mdi:counter",       true},  vRxFrames},
	{{"wisafe_rx_timeouts",   "WiSafe RX Timeouts",     "None", "None", "mdi:counter",       true},  vRxTimeouts},
	{{"wisafe_rx_bad_length", "WiSafe RX Bad Length",   "None", "None", "mdi:counter",       true},  vRxBadLen},
	{{"wisafe_rx_short",      "WiSafe RX Short Packets","None", "None", "mdi:counter",       true},  vRxShort},
	{{"wisafe_tx_sent",       "WiSafe Messages Sent",   "None", "None", "mdi:counter",       true},  vTxSent},
	{{"wisafe_tx_dropped",    "WiSafe Messages Dropped","None", "None", "mdi:counter",       true},  vTxDropped},
	{{"wisafe_tx_queue",      "WiSafe TX Queue",        "None", "None", "mdi:tray-full",     true},  vTxQueue},
};

int bridgeSensorCount() {
	return (int)(sizeof(entries) / sizeof(entries[0]));
}

const bridge_sensor_t* bridgeSensor(int index) {
	if (index < 0 || index >= bridgeSensorCount())
		return NULL;
	return &entries[index].info;
}

bool bridgeSensorValue(int index, char* out, size_t outSize) {
	if (index < 0 || index >= bridgeSensorCount() || outSize == 0)
		return false;
	out[0] = 0;
	return entries[index].value(out, outSize);
}
