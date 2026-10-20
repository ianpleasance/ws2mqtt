#ifndef MQTT_LOG_H
#define MQTT_LOG_H 1

#include <stdint.h>
#include <stddef.h>

// Ring buffers of recent MQTT messages for the web UI.
//
// Messages are split into two rings so that the frequent housekeeping traffic
// (discovery configs and the bridge sensors published every few seconds) can
// not push the interesting messages (alarms, device states, commands) out.
#define MQTT_LOG_EVENT_SIZE 64
#define MQTT_LOG_HOUSEKEEPING_SIZE 32

#define MQTT_LOG_TOPIC_LEN 96
#define MQTT_LOG_PAYLOAD_LEN 80

enum MqttLogCategory : uint8_t {
	MQTT_CAT_EVENT = 0,
	MQTT_CAT_HOUSEKEEPING = 1
};

typedef struct {
	uint32_t ts;           // millis() when the message was sent / received
	uint16_t payloadLen;   // full payload length (payload below may be cut short)
	uint8_t category;
	bool incoming;
	bool retained;
	bool ok;               // false if the publish failed
	char topic[MQTT_LOG_TOPIC_LEN];
	char payload[MQTT_LOG_PAYLOAD_LEN];
} mqtt_log_entry_t;

// Allocate the buffers. Call once at start up. If allocation fails the log is
// simply disabled and everything else carries on working.
void mqttLogBegin();

void mqttLogAdd(MqttLogCategory category, bool incoming, const char* topic,
	const char* payload, size_t payloadLen, bool retained, bool ok);

// Number of entries currently held in a ring
size_t mqttLogCount(MqttLogCategory category);

// Entry by age, 0 = newest. Returns NULL if out of range.
const mqtt_log_entry_t* mqttLogGet(MqttLogCategory category, size_t index);

// Publish through the shared MQTT client, recording the message in the log and
// updating the publish statistics. Returns true if the publish succeeded.
bool mqttPublish(const char* topic, const char* payload, bool retained, MqttLogCategory category);

#endif // MQTT_LOG_H
