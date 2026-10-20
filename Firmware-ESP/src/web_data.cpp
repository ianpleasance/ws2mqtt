#include "Arduino.h"
#include <WiFi.h>
#include <PubSubClient.h>

#include "web_data.h"
#include "config.h"
#include "credentials.h"
#include "version.h"
#include "sysinfo.h"
#include "module_info.h"
#include "device_runtime.h"
#include "mqtt_log.h"
#include "wisafe2_packets.h"
#include "wisafe2_tx.h"
#include "homeassistant.h"
#include "device_models.h"

extern PubSubClient mqttClient;
extern int ownSid;

// ---------------------------------------------------------------------------
// Minimal streaming JSON writer
// ---------------------------------------------------------------------------
class JW {
public:
	explicit JW(Print& output) : o(output), depth(0) {}

	void obj(const char* key = NULL)  { open(key, '{', '}'); }
	void arr(const char* key = NULL)  { open(key, '[', ']'); }
	void end() {
		if (depth > 0) {
			o.print(closer[--depth]);
		}
	}

	void str(const char* key, const char* value) {
		sep(key);
		string(value);
	}
	void num(const char* key, long long value) {
		sep(key);
		o.printf("%lld", value);
	}
	void unum(const char* key, unsigned long long value) {
		sep(key);
		o.printf("%llu", value);
	}
	void boolean(const char* key, bool value) {
		sep(key);
		o.print(value ? "true" : "false");
	}
	void nul(const char* key) {
		sep(key);
		o.print("null");
	}
	// "0x" followed by uppercase hex, padded to the given number of digits
	void hex(const char* key, uint32_t value, int digits) {
		sep(key);
		o.printf("\"0x%0*X\"", digits, (unsigned int)value);
	}
	// Age in milliseconds of a millis() timestamp, or null if never
	void age(const char* key, uint32_t timestamp) {
		if (timestamp == 0)
			nul(key);
		else
			unum(key, (unsigned long long)(uint32_t)(millis() - timestamp));
	}

private:
	Print& o;
	bool first[8];
	char closer[8];
	int depth;

	void sep(const char* key) {
		if (depth > 0) {
			if (!first[depth - 1])
				o.print(',');
			first[depth - 1] = false;
		}
		if (key != NULL) {
			string(key);
			o.print(':');
		}
	}

	void open(const char* key, char opener, char closing) {
		if (depth >= 8)
			return;
		sep(key);
		o.print(opener);
		first[depth] = true;
		closer[depth] = closing;
		depth++;
	}

	void string(const char* s) {
		o.print('"');
		for (; s != NULL && *s; s++) {
			unsigned char c = (unsigned char)*s;
			switch (c) {
			case '"':  o.print("\\\""); break;
			case '\\': o.print("\\\\"); break;
			case '\n': o.print("\\n"); break;
			case '\r': o.print("\\r"); break;
			case '\t': o.print("\\t"); break;
			default:
				if (c < 0x20)
					o.printf("\\u%04x", c);
				else
					o.write(c);
			}
		}
		o.print('"');
	}
};

// ---------------------------------------------------------------------------
// /data/system.json
// ---------------------------------------------------------------------------
void writeSystemJson(Print& out) {
	JW j(out);
	char text[48];

	j.obj();
	j.str("firmware", FIRMWARE_VERSION);
	j.str("build", buildTimestamp());
	j.unum("uptime_s", uptimeSeconds());
	j.str("reset_reason", resetReasonString(esp_reset_reason()));

	j.obj("chip");
	j.str("model", ESP.getChipModel());
	j.num("revision", ESP.getChipRevision());
	j.num("cores", ESP.getChipCores());
	j.num("cpu_mhz", ESP.getCpuFreqMHz());
	j.unum("flash_bytes", ESP.getFlashChipSize());
	j.unum("sketch_bytes", ESP.getSketchSize());
	j.str("sdk", ESP.getSdkVersion());
	j.end();

	j.obj("heap");
	j.unum("total", ESP.getHeapSize());
	j.unum("free", ESP.getFreeHeap());
	j.unum("min_free", ESP.getMinFreeHeap());
	j.end();

	j.obj("bridge");
	j.hex("device_id", DEVICE_ID, 6);
	snprintf(text, sizeof(text), "ws2mqtt_%08x", DEVICE_ID);
	j.str("name", text);
	j.end();

	j.obj("wifi");
	bool wifiUp = (WiFi.status() == WL_CONNECTED);
	j.boolean("connected", wifiUp);
	const char* hostname = WiFi.getHostname();
	j.str("hostname", hostname != NULL ? hostname : "");
	j.str("ip", WiFi.localIP().toString().c_str());
	j.str("mac", WiFi.macAddress().c_str());
	j.str("ssid", WiFi.SSID().c_str());
	j.str("bssid", WiFi.BSSIDstr().c_str());
	j.num("rssi", WiFi.RSSI());
	j.num("channel", WiFi.channel());
	j.end();

	j.obj("mqtt");
	j.str("host", MQTT_HOST);
	j.boolean("connected", mqttClient.connected());
	j.num("state", mqttClient.state());
	j.str("state_text", mqttStateString(mqttClient.state()));
	j.unum("connect_attempts", mqttStats.connectAttempts);
	j.unum("connects", mqttStats.connects);
	j.age("last_connect_age_ms", mqttStats.lastConnectMs);
	if (mqttStats.connectAttempts > mqttStats.connects) {
		j.num("last_fail_state", mqttStats.lastFailState);
		j.str("last_fail_text", mqttStateString(mqttStats.lastFailState));
	}
	j.unum("published", mqttStats.published);
	j.unum("publish_failed", mqttStats.publishFailed);
	j.unum("received", mqttStats.received);
	j.end();

	j.end();
}

// ---------------------------------------------------------------------------
// /data/module.json
// ---------------------------------------------------------------------------
void writeModuleJson(Print& out) {
	JW j(out);

	j.obj();
	j.hex("gateway_device_id", DEVICE_ID, 6);

	if (ownSid < 0) {
		j.nul("own_sid");
		j.nul("paired");
	} else {
		j.num("own_sid", ownSid);
		// SID 0x40 is above the maximum SID, which means "not connected"
		j.boolean("paired", ownSid != 0x40);
	}

	if (moduleInfo.diagReceived) {
		j.obj("diag");
		j.num("sid", moduleInfo.sid);
		j.unum("device_id", moduleInfo.device_id);
		j.boolean("initialised", moduleInfo.device_id != 0);
		j.num("battery1", moduleInfo.battery1);
		j.num("battery2", moduleInfo.battery2);
		j.num("rssi", moduleInfo.rssi);
		j.num("firmware_version", moduleInfo.firmware_version);
		j.hex("flags", moduleInfo.flags, 2);
		j.num("radio_fault_count", moduleInfo.radio_fault_count);
		j.age("age_ms", moduleInfo.diagAtMs);
		j.end();
	} else {
		j.nul("diag");
	}

	if (moduleInfo.sidmapReceived) {
		j.obj("sidmap");
		char hexmap[20];
		snprintf(hexmap, sizeof(hexmap), "%08lx%08lx",
			(unsigned long)(moduleInfo.sidmap >> 32), (unsigned long)(moduleInfo.sidmap & 0xFFFFFFFFUL));
		j.str("hex", hexmap);
		j.arr("sids");
		for (int sid = 0; sid < 64; sid++)
			if ((moduleInfo.sidmap >> sid) & 0x01)
				j.num(NULL, sid);
		j.end();
		j.age("age_ms", moduleInfo.sidmapAtMs);
		j.end();
	} else {
		j.nul("sidmap");
	}

	j.obj("welcome");
	j.unum("count", moduleInfo.welcomeCount);
	j.age("age_ms", moduleInfo.lastWelcomeMs);
	j.end();

	j.obj("rx");
	j.unum("frames", moduleInfo.rxFrames);
	j.unum("frame_timeouts", moduleInfo.rxFrameTimeouts);
	j.unum("bad_length", moduleInfo.rxBadLength);
	j.unum("short_packets", moduleInfo.rxShortPackets);
	j.age("age_ms", moduleInfo.lastRxMs);
	j.end();

	j.obj("tx");
	j.unum("sent", moduleInfo.txSent);
	j.unum("dropped", moduleInfo.txDropped);
	j.num("queued", txQueueDepth());
	j.age("age_ms", moduleInfo.lastTxMs);
	j.end();

	j.end();
}

// ---------------------------------------------------------------------------
// /data/devices.json
// ---------------------------------------------------------------------------
void writeDevicesJson(Print& out) {
	JW j(out);
	int count = 0;
	for (int sid = 0; sid < RUNTIME_MAX_SID; sid++)
		if (runtimeBySid(sid) != NULL)
			count++;

	j.obj();
	j.num("count", count);
	j.arr("devices");

	for (int sid = 0; sid < RUNTIME_MAX_SID; sid++) {
		const device_runtime_t* d = runtimeBySid(sid);
		if (d == NULL)
			continue;

		j.obj();
		j.num("sid", d->sid);
		j.unum("device_id", d->device_id);
		j.hex("model_id", d->model, 4);
		j.str("model", modelString(d->model));
		j.hex("type_id", d->device_type, 2);
		j.str("type", deviceTypeInfo(d->device_type)->name);

		if (d->alarmKnown)
			j.boolean("alarm", d->alarm);
		else
			j.nul("alarm");

		if (d->haveState) {
			j.obj("state");
			j.boolean("generic_error", d->generic_error);
			j.boolean("docked", d->docked);
			j.boolean("sensor_battery", d->sensor_battery);
			j.boolean("radio_module_battery", d->radio_module_battery);
			j.end();
		} else {
			j.nul("state");
		}

		if (d->haveDiag) {
			j.obj("diag");
			j.num("battery1", d->battery1);
			j.num("battery2", d->battery2);
			j.num("rssi", d->rssi);
			j.num("firmware_version", d->firmware_version);
			j.hex("flags", d->flags, 2);
			j.num("radio_fault_count", d->radio_fault_count);
			j.end();
		} else {
			j.nul("diag");
		}

		j.age("last_seen_age_ms", d->lastSeenMs);
		j.age("last_alarm_age_ms", d->lastAlarmMs);
		j.age("last_button_age_ms", d->lastButtonMs);
		j.end();
	}

	j.end();
	j.end();
}

// ---------------------------------------------------------------------------
// /data/mqtt.json
// ---------------------------------------------------------------------------
static void writeMqttEntry(JW& j, const mqtt_log_entry_t* e) {
	j.obj();
	j.age("age_ms", e->ts == 0 ? 1 : e->ts);
	j.str("dir", e->incoming ? "in" : "out");
	j.str("category", e->category == MQTT_CAT_HOUSEKEEPING ? "housekeeping" : "event");
	j.boolean("retained", e->retained);
	j.boolean("ok", e->ok);
	j.str("topic", e->topic);
	j.str("payload", e->payload);
	j.unum("payload_len", e->payloadLen);
	j.end();
}

void writeMqttJson(Print& out, bool includeHousekeeping) {
	JW j(out);

	size_t nEvent = mqttLogCount(MQTT_CAT_EVENT);
	size_t nHouse = includeHousekeeping ? mqttLogCount(MQTT_CAT_HOUSEKEEPING) : 0;

	j.obj();
	j.boolean("includes_housekeeping", includeHousekeeping);
	j.arr("messages");

	// Merge the two rings, newest first
	size_t i = 0, h = 0;
	while (i < nEvent || h < nHouse) {
		const mqtt_log_entry_t* e = (i < nEvent) ? mqttLogGet(MQTT_CAT_EVENT, i) : NULL;
		const mqtt_log_entry_t* k = (h < nHouse) ? mqttLogGet(MQTT_CAT_HOUSEKEEPING, h) : NULL;

		if (e == NULL && k == NULL)
			break;

		if (k == NULL || (e != NULL && (int32_t)(e->ts - k->ts) >= 0)) {
			writeMqttEntry(j, e);
			i++;
		} else {
			writeMqttEntry(j, k);
			h++;
		}
	}

	j.end();
	j.end();
}
