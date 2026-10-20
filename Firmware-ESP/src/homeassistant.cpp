#include "homeassistant.h"
#include "config.h"
#include "wifi_inc.h"
#include "wisafe2_packets.h"
#include "wisafe2_tx.h"
#include "mqtt_log.h"
#include "sysinfo.h"
#include "version.h"
#include "bridge_sensors.h"

extern PubSubClient mqttClient;

unsigned long lastSensorPush = 0;

// Serialise a discovery document, refusing to publish a config that was
// silently truncated (ArduinoJson drops members once the document is full)
static bool serializeDiscovery(JsonDocument& doc, char* out, size_t outSize) {
	if (doc.overflowed()) {
		Serial.println("Discovery config too large for JSON document, not publishing");
		return false;
	}
	if (serializeJson(doc, out, outSize) >= outSize) {
		Serial.println("Discovery config too large for output buffer, not publishing");
		return false;
	}
	return true;
}

void announceAllDevices() {
	announceMQTTBridge();
	announceMQTTBridgeEntities();

	getAllDevicesFromDB(announceMQTTDevice);
}

void publishAllCachedDeviceStates() {
	getAllDeviceStatesFromDB(sendMQTTDeviceState);
}

void announceMQTTBridge() {
	StaticJsonDocument<768> doc;

	// Create device
	JsonObject dev = doc.createNestedObject("dev");
	addBridgeDescription(dev);

	doc["atype"] = "trigger";
	doc["type"] = "received";
	doc["subtype"] = "alarm";

	char eventtopic[200];
	sprintf(eventtopic, "ws2mqtt/bridge_%08x/event", DEVICE_ID);
	doc["t"] = eventtopic;
	doc["pl"] = "alarm";

	char msg[800];
	if (!serializeDiscovery(doc, msg, sizeof(msg)))
		return;

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/device_automation/ws2mqtt_bridge_%08x/alarm/config", DEVICE_ID);
		mqttPublish(topic, msg, true, MQTT_CAT_HOUSEKEEPING);
	}
}

// Sensors whose keys were renamed (now prefixed wifi_). The old discovery
// configs and retained values are cleared so Home Assistant drops the old entities.
static void removeLegacyBridgeSensors() {
	static const char* legacy[] = {"ip_addr", "rssi", "mac_addr", "ssid"};
	if (!mqttClient.connected())
		return;
	for (size_t i = 0; i < sizeof(legacy) / sizeof(legacy[0]); i++) {
		char topic[200];
		snprintf(topic, sizeof(topic), HA_AUTODISCOVERY_PREFIX"/sensor/ws2mqtt_bridge_%08x/%s/config", DEVICE_ID, legacy[i]);
		mqttPublish(topic, "", true, MQTT_CAT_HOUSEKEEPING);
		snprintf(topic, sizeof(topic), "ws2mqtt/bridge_%08x/%s", DEVICE_ID, legacy[i]);
		mqttPublish(topic, "", true, MQTT_CAT_HOUSEKEEPING);
	}
}

void announceMQTTBridgeEntities() {
	removeLegacyBridgeSensors();

	// Test
	announceMQTTBridgeButtonEntity((char*)"Test All Devices", (char*)"test_devices", false, true, (char*)"mdi:bell-cog"); //Test All Button
	announceMQTTBridgeButtonEntity((char*)"Test Smoke Devices", (char*)"test_smoke", false, false, (char*)"mdi:bell-cog"); //Test Smoke Button
	announceMQTTBridgeButtonEntity((char*)"Test CO Devices", (char*)"test_co", false, false, (char*)"mdi:bell-cog"); //Test CO Button

	// Silence
#ifdef ENABLE_SILENCE_ALARM
	announceMQTTBridgeButtonEntity((char*)"Silence Smoke Devices", (char*)"silence_smoke", false, false, (char*)"mdi:bell-sleep"); //Silence Smoke Button
	announceMQTTBridgeButtonEntity((char*)"Silence CO Devices", (char*)"silence_co", false, false, (char*)"mdi:bell-sleep"); //Silence CO Button
#endif

	// Emergency
#ifdef ENABLE_TRIGGER_ALARM
	announceMQTTBridgeButtonEntity((char*)"Emergency Smoke", (char*)"emergency_smoke", false, false, (char*)"mdi:bell-sleep"); //Emergency Smoke Button
	announceMQTTBridgeButtonEntity((char*)"Emergency CO", (char*)"emergency_co", false, false, (char*)"mdi:bell-sleep"); //Emergency CO Button
#endif

	// Restart button
	announceMQTTBridgeButtonEntity((char*)"ESP Restart", (char*)"esp_restart", true, false, (char*)"mdi:restart"); // Restart Button

	// ESP32, network, MQTT and WiSafe module sensors (see bridge_sensors.cpp)
	for (int i = 0; i < bridgeSensorCount(); i++) {
		const bridge_sensor_t* s = bridgeSensor(i);
		announceMQTTBridgeSensorEntity((char*)s->name, (char*)s->key, (char*)s->devClass, (char*)s->unit, true, s->enabled, (char*)s->icon);
	}
}

void announceMQTTBridgeButtonEntity(char* name, char* command, bool diagnostic, bool enabled, char* icon) {
	StaticJsonDocument<768> doc;

	char avtytopic[200];
	sprintf(avtytopic, "ws2mqtt/bridge_%08x/state", DEVICE_ID);
	doc["avty_t"] = avtytopic;

	JsonObject dev = doc.createNestedObject("dev");
	addBridgeDescription(dev);

	doc["name"] = name;
	if (diagnostic)
		doc["entity_category"] = "diagnostic";
	if (!enabled)
		doc["en"] = false;
	doc["icon"] = icon;
	char cmdtopic[200];
	sprintf(cmdtopic, "ws2mqtt/bridge_%08x/command", DEVICE_ID);
	doc["cmd_t"] = cmdtopic;
	doc["cmd_tpl"] = command;

	char uniq_id[128];
	sprintf(uniq_id, "ws2mqtt_bridge_command_%s", command);
	doc["uniq_id"] = uniq_id;

	char msg[800];
	if (!serializeDiscovery(doc, msg, sizeof(msg)))
		return;

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/button/ws2mqtt_bridge_%08x/command_%s/config", DEVICE_ID, command);
		Serial.println(topic);
		mqttPublish(topic, msg, true, MQTT_CAT_HOUSEKEEPING);
	}
}

void announceMQTTBridgeSensorEntity(char* name, char* sensor, char* dev_class, char* unit, bool diagnostic, bool enabled, char* icon) {
	StaticJsonDocument<768> doc;

	char avtytopic[200];
	sprintf(avtytopic, "ws2mqtt/bridge_%08x/state", DEVICE_ID);
	doc["avty_t"] = avtytopic;

	JsonObject dev = doc.createNestedObject("dev");
	addBridgeDescription(dev);

	doc["name"] = name;
	if (diagnostic)
		doc["entity_category"] = "diagnostic";
	if (!enabled)
		doc["en"] = false;
	doc["icon"] = icon;
	if (strcmp(dev_class, "None") != 0)
		doc["dev_cla"] = dev_class;
	if (strcmp(unit, "None") != 0)
		doc["unit_of_meas"] = unit;
	char stattopic[200];
	sprintf(stattopic, "ws2mqtt/bridge_%08x/%s", DEVICE_ID, sensor);
	doc["stat_t"] = stattopic;

	char uniq_id[128];
	sprintf(uniq_id, "ws2mqtt_bridge_sensor_%s", sensor);
	doc["uniq_id"] = uniq_id;

	char msg[800];
	if (!serializeDiscovery(doc, msg, sizeof(msg)))
		return;

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/sensor/ws2mqtt_bridge_%08x/%s/config", DEVICE_ID, sensor);
		Serial.println(topic);
		mqttPublish(topic, msg, true, MQTT_CAT_HOUSEKEEPING);
	}

}

void addBridgeDescription(JsonObject dev) {
	char devid[32];
	sprintf(devid, "ws2mqtt_bridge_%08x", DEVICE_ID);
	dev["mf"] = "Tho85";
	dev["mdl"] = "WS2MQTT bridge";
	dev["sw"] = FIRMWARE_VERSION;
	dev["ids"] = devid;

	char devname[128];
	sprintf(devname, "WS2MQTT bridge %08x", DEVICE_ID);
	dev["name"] = devname;
}

void sendMQTTBridgeEvent() {
	char eventtopic[200];
	sprintf(eventtopic, "ws2mqtt/bridge_%08x/event", DEVICE_ID);
	if (mqttClient.connected())
		mqttPublish(eventtopic, "alarm", false, MQTT_CAT_EVENT);
}

// Last value sent for each bridge sensor. A value is sent again when it changes,
// or every BRIDGE_SENSOR_HEARTBEAT seconds, and everything is re-sent after the
// MQTT connection is re-established.
#define BRIDGE_SENSOR_MAX 64
#define BRIDGE_SENSOR_VALUE_LEN 200
#define BRIDGE_SENSOR_HEARTBEAT 300
static char bridgeLastValue[BRIDGE_SENSOR_MAX][BRIDGE_SENSOR_VALUE_LEN];
static bool bridgeSent[BRIDGE_SENSOR_MAX];
static unsigned long bridgeSentAt[BRIDGE_SENSOR_MAX];
static uint32_t bridgeSeenConnects = 0;

void loopBridgeSensors () {
	if (millis() - lastSensorPush <= BRIDGE_SENSOR_INTERVAL * 1000)
		return;
	lastSensorPush = millis();

	if (!mqttClient.connected())
		return;

	if (mqttStats.connects != bridgeSeenConnects) {
		bridgeSeenConnects = mqttStats.connects;
		memset(bridgeSent, 0, sizeof(bridgeSent));
	}

	int count = bridgeSensorCount();
	if (count > BRIDGE_SENSOR_MAX)
		count = BRIDGE_SENSOR_MAX;

	char value[BRIDGE_SENSOR_VALUE_LEN];
	for (int i = 0; i < count; i++) {
		if (!bridgeSensorValue(i, value, sizeof(value)))
			continue;
		bool changed = !bridgeSent[i] || strcmp(value, bridgeLastValue[i]) != 0;
		bool due = millis() - bridgeSentAt[i] > BRIDGE_SENSOR_HEARTBEAT * 1000UL;
		if (!changed && !due)
			continue;
		sendMQTTBridgeSensor((char*)bridgeSensor(i)->key, value);
		strcpy(bridgeLastValue[i], value);
		bridgeSent[i] = true;
		bridgeSentAt[i] = millis();
	}
}

void sendMQTTBridgeSensor (char* sensor, char* sensor_value) {

	if (mqttClient.connected()) {
		
		char init_topic[200];
		sprintf(init_topic, "ws2mqtt/bridge_%08x/%s", DEVICE_ID, sensor);
		mqttPublish(init_topic, sensor_value, true, MQTT_CAT_HOUSEKEEPING);
	}
}

void announceMQTTDevice(device_t device) {
	const device_type_info_t* type = deviceTypeInfo(device.device_type);
	announceMQTTBinarySensor(device, (char *)type->haClass, (char *)type->entityName, (char *)"alarm", false);
	announceMQTTBinarySensor(device, (char *)"battery", (char *)"Battery", (char *)"sensor_battery", true);
	announceMQTTBinarySensor(device, (char *)"battery", (char *)"Radio module battery", (char *)"radio_module_battery", true);
	announceMQTTBinarySensor(device, (char *)"plug", (char *)"Docked", (char *)"docked", true);
	announceMQTTBinarySensor(device, (char *)"problem", (char *)"Generic error", (char *)"generic_error", true);

	announceMQTTButtonEvent(device);
}

void addDeviceDescription(JsonObject dev, device_t device) {
	char devid[32];
	snprintf(devid, sizeof(devid), "ws2mqtt_%u", (unsigned int)device.device_id);
	dev["ids"] = devid;

	char* manufacturer = (char *)"FireAngel";
	char* model = (char *)modelString(device.model);
	dev["mf"] = manufacturer;
	dev["mdl"] = model;

	char bridgeid[32];
	sprintf(bridgeid, "ws2mqtt_bridge_%08x", DEVICE_ID);
	dev["via_device"] = bridgeid;

	char devname[128];
	snprintf(devname, sizeof(devname), "%s %s %u", manufacturer, model, (unsigned int)device.device_id);
	dev["name"] = devname;
}

void announceMQTTBinarySensor(
	device_t device,
	char* device_class,
	char* name,
	char* short_name,
	bool diagnostic
) {
	StaticJsonDocument<768> doc;

	char avtytopic[200];
	sprintf(avtytopic, "ws2mqtt/bridge_%08x/state", DEVICE_ID);
	doc["avty_t"] = avtytopic;

	JsonObject dev = doc.createNestedObject("dev");
	addDeviceDescription(dev, device);

	char sensor_name[128];
	char* model = (char *)modelString(device.model);
	snprintf(sensor_name, sizeof(sensor_name), "%s %u %s", model, (unsigned int)device.device_id, name);
	doc["name"] = sensor_name;

	doc["dev_cla"] = device_class;
	if (diagnostic)
		doc["entity_category"] = "diagnostic";

	char stat_t[128];
	snprintf(stat_t, sizeof(stat_t), "ws2mqtt/%u/%s/state", (unsigned int)device.device_id, short_name);
	doc["stat_t"] = stat_t;

	char uniq_id[128];
	snprintf(uniq_id, sizeof(uniq_id), "%u_%s_ws2mqtt", (unsigned int)device.device_id, short_name);
	doc["uniq_id"] = uniq_id;

	char msg[800];
	if (!serializeDiscovery(doc, msg, sizeof(msg)))
		return;

	if (mqttClient.connected()) {
		char topic[200];
		snprintf(topic, sizeof(topic), HA_AUTODISCOVERY_PREFIX"/binary_sensor/ws2mqtt_%u/%s/config", (unsigned int)device.device_id, short_name);
		Serial.println(topic);
		mqttPublish(topic, msg, true, MQTT_CAT_HOUSEKEEPING);
	}
}

void sendMQTTState(uint32_t device_id, char* short_name, bool state) {
	if (mqttClient.connected()) {
		char topic[200];
		snprintf(topic, sizeof(topic), "ws2mqtt/%u/%s/state", (unsigned int)device_id, short_name);
		mqttPublish(topic, state ? "ON" : "OFF", false, MQTT_CAT_EVENT);
	}
}

void sendMQTTDeviceState(device_state_t ds) {
	sendMQTTState(ds.device_id, (char*)"generic_error", ds.generic_error);
	sendMQTTState(ds.device_id, (char*)"docked", ds.docked);
	sendMQTTState(ds.device_id, (char*)"sensor_battery", ds.sensor_battery);
	sendMQTTState(ds.device_id, (char*)"radio_module_battery", ds.radio_module_battery);
}

void announceMQTTButtonEvent(device_t device) {
	StaticJsonDocument<768> doc;

	// Create device
	JsonObject dev = doc.createNestedObject("dev");
	addDeviceDescription(dev, device);

	doc["atype"] = "trigger";
	doc["type"] = "button_short_press";
	doc["subtype"] = "button_1";

	char t[128];
	snprintf(t, sizeof(t), "ws2mqtt/%u/event", (unsigned int)device.device_id);
	doc["t"] = t;
	doc["pl"] = "test_button";

	char msg[800];
	if (!serializeDiscovery(doc, msg, sizeof(msg)))
		return;

	if (mqttClient.connected()) {
		char topic[200];
		snprintf(topic, sizeof(topic), HA_AUTODISCOVERY_PREFIX"/device_automation/ws2mqtt_%u/test_button/config", (unsigned int)device.device_id);
		mqttPublish(topic, msg, true, MQTT_CAT_HOUSEKEEPING);
	}
}

void sendMQTTButtonEvent(uint32_t device_id) {
	if (mqttClient.connected()) {
		char topic[200];
		snprintf(topic, sizeof(topic), "ws2mqtt/%u/event", (unsigned int)device_id);
		mqttPublish(topic, "test_button", false, MQTT_CAT_EVENT);
	}
}

void removeMQTTDevice(device_t device) {
	removeMQTTBinarySensor(device, (char *)"alarm");

	removeMQTTBinarySensor(device, (char *)"sensor_battery");
	removeMQTTBinarySensor(device, (char *)"radio_module_battery");
	removeMQTTBinarySensor(device, (char *)"docked");
	removeMQTTBinarySensor(device, (char *)"generic_error");

	removeMQTTButtonEvent(device);
}

void removeMQTTBinarySensor(device_t device, char* short_name) {
	if (mqttClient.connected()) {
		char topic[200];
		snprintf(topic, sizeof(topic), HA_AUTODISCOVERY_PREFIX"/binary_sensor/ws2mqtt_%u/%s/config", (unsigned int)device.device_id, short_name);
		Serial.println(topic);
		mqttPublish(topic, "", true, MQTT_CAT_HOUSEKEEPING);
	} 
	
}

void removeMQTTButtonEvent(device_t device) {
	if (mqttClient.connected()) {
		char topic[200];
		snprintf(topic, sizeof(topic), HA_AUTODISCOVERY_PREFIX"/device_automation/ws2mqtt_%u/test_button/config", (unsigned int)device.device_id);
		mqttPublish(topic, "", true, MQTT_CAT_HOUSEKEEPING);
	}
}

void handleMQTTCommand(char* command) {
	if (strcmp(command, "test_devices") == 0) {
		sendTestButtonMsg(DEVICE_TYPE_ALL);
	} 
	else if (strcmp(command, "test_smoke") == 0) { 
		sendTestButtonMsg(DEVICE_TYPE_SMOKE);
	} 
	else if (strcmp(command, "test_co") == 0) { 
		sendTestButtonMsg(DEVICE_TYPE_CO);
	} 
#ifdef ENABLE_SILENCE_ALARM
	else if (strcmp(command, "silence_smoke") == 0) {
		sendSilenceButtonMsg(DEVICE_TYPE_SMOKE);
	} 
	else if (strcmp(command, "silence_co") == 0) {
		sendSilenceButtonMsg(DEVICE_TYPE_CO);
	}
#endif
#ifdef ENABLE_TRIGGER_ALARM
	else if (strcmp(command, "emergency_smoke") == 0) {
		sendEmergencyButtonMsg(DEVICE_TYPE_SMOKE);
	}
	else if (strcmp(command, "emergency_co") == 0) {
		sendEmergencyButtonMsg(DEVICE_TYPE_CO);
	}
#endif
	else if (strcmp(command, "esp_restart") == 0) {
		ESP.restart();
	}

}
