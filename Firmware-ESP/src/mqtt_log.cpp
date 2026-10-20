#include "Arduino.h"
#include <PubSubClient.h>

#include "mqtt_log.h"
#include "sysinfo.h"

extern PubSubClient mqttClient;

static mqtt_log_entry_t* rings[2] = { NULL, NULL };
static const size_t ringSize[2] = { MQTT_LOG_EVENT_SIZE, MQTT_LOG_HOUSEKEEPING_SIZE };
static size_t ringHead[2] = { 0, 0 };   // next slot to write
static size_t ringCount[2] = { 0, 0 };

void mqttLogBegin() {
	for (int i = 0; i < 2; i++) {
		if (rings[i] != NULL)
			continue;

		rings[i] = (mqtt_log_entry_t*)calloc(ringSize[i], sizeof(mqtt_log_entry_t));
		if (rings[i] == NULL)
			Serial.println("Could not allocate MQTT log, log disabled");
	}
}

void mqttLogAdd(MqttLogCategory category, bool incoming, const char* topic,
	const char* payload, size_t payloadLen, bool retained, bool ok) {

	int r = (category == MQTT_CAT_HOUSEKEEPING) ? 1 : 0;
	if (rings[r] == NULL)
		return;

	mqtt_log_entry_t* e = &rings[r][ringHead[r]];
	ringHead[r] = (ringHead[r] + 1) % ringSize[r];
	if (ringCount[r] < ringSize[r])
		ringCount[r]++;

	e->ts = millis();
	e->category = category;
	e->incoming = incoming;
	e->retained = retained;
	e->ok = ok;
	e->payloadLen = payloadLen > 0xFFFF ? 0xFFFF : (uint16_t)payloadLen;

	strncpy(e->topic, topic ? topic : "", sizeof(e->topic) - 1);
	e->topic[sizeof(e->topic) - 1] = '\0';

	size_t copyLen = payloadLen;
	if (payload == NULL)
		copyLen = 0;
	if (copyLen > sizeof(e->payload) - 1)
		copyLen = sizeof(e->payload) - 1;
	if (copyLen > 0)
		memcpy(e->payload, payload, copyLen);
	e->payload[copyLen] = '\0';
}

size_t mqttLogCount(MqttLogCategory category) {
	int r = (category == MQTT_CAT_HOUSEKEEPING) ? 1 : 0;
	return rings[r] == NULL ? 0 : ringCount[r];
}

const mqtt_log_entry_t* mqttLogGet(MqttLogCategory category, size_t index) {
	int r = (category == MQTT_CAT_HOUSEKEEPING) ? 1 : 0;
	if (rings[r] == NULL || index >= ringCount[r])
		return NULL;

	size_t pos = (ringHead[r] + ringSize[r] - 1 - index) % ringSize[r];
	return &rings[r][pos];
}

bool mqttPublish(const char* topic, const char* payload, bool retained, MqttLogCategory category) {
	const char* body = payload ? payload : "";
	bool ok = mqttClient.publish(topic, body, retained);

	if (ok)
		mqttStats.published++;
	else
		mqttStats.publishFailed++;

	mqttLogAdd(category, false, topic, body, strlen(body), retained, ok);
	return ok;
}
