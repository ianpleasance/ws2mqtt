#include "homeassistant.h"
#include "config.h"
#include "wifi_inc.h"
#include "wisafe2_packets.h"
#include "wisafe2_tx.h"

extern PubSubClient mqttClient;

unsigned long lastSensorPush = 0;

void announceAllDevices() {
	announceMQTTBridge();
	announceMQTTBridgeEntities();

	getAllDevicesFromDB(announceMQTTDevice);
}

void publishAllCachedDeviceStates() {
	getAllDeviceStatesFromDB(sendMQTTDeviceState);
}

void announceMQTTBridge() {
	StaticJsonDocument<500> doc;

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

	char msg[500];
	serializeJson(doc, msg, 500);

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/device_automation/ws2mqtt_bridge_%08x/alarm/config", DEVICE_ID);
		mqttClient.publish(topic, msg, true);
	}
}

void announceMQTTBridgeEntities() {
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

	// WiFi Sensors
	announceMQTTBridgeSensorEntity((char*)"IP Address", (char*)"ip_addr", (char*)"None", (char*)"None", true, true, (char*)"mdi:ip-network"); //IP Address Sensor
	announceMQTTBridgeSensorEntity((char*)"RSSI", (char*)"rssi", (char*)"signal_strength", (char*)"dBm", true, true, (char*)"mdi:wifi"); //Wifi RSSI
	announceMQTTBridgeSensorEntity((char*)"MAC Address", (char*)"mac_addr", (char*)"None", (char*)"None", true, true, (char*)"mdi:wifi"); //Wifi Mac Address
	announceMQTTBridgeSensorEntity((char*)"SSID", (char*)"ssid", (char*)"None", (char*)"None", true, true, (char*)"mdi:wifi"); //Wifi SSID

	// Restart
	announceMQTTBridgeButtonEntity((char*)"ESP Restart", (char*)"esp_restart", true, false, (char*)"mdi:restart"); // Restart Button
	announceMQTTBridgeSensorEntity((char*)"ESP Uptime", (char*)"esp_uptime", (char*)"duration", (char*)"s", true, false, (char*)"mdi:clock"); // Time since last restart
	announceMQTTBridgeSensorEntity((char*)"ESP Restart Reason", (char*)"esp_reset_reason", (char*)"None", (char*)"None", true, false, (char*)"mdi:restart-alert");

	// ESP Info
	announceMQTTBridgeSensorEntity((char*)"ESP Model", (char*)"esp_model", (char*)"None", (char*)"None", true, false, (char*)"mdi:chip"); // ESP Chip Model
}

void announceMQTTBridgeButtonEntity(char* name, char* command, bool diagnostic, bool enabled, char* icon) {
	StaticJsonDocument<500> doc;

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

	char msg[500];
	serializeJson(doc, msg, 500);

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/button/ws2mqtt_bridge_%08x/command_%s/config", DEVICE_ID, command);
		Serial.println(topic);
		mqttClient.publish(topic, msg, true);
	}
}

void announceMQTTBridgeSensorEntity(char* name, char* sensor, char* dev_class, char* unit, bool diagnostic, bool enabled, char* icon) {
	StaticJsonDocument<500> doc;

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
	if (dev_class!="None")
		doc["dev_cla"] = dev_class;
	if (unit!="None")
		doc["unit_of_meas"] = unit;
	char stattopic[200];
	sprintf(stattopic, "ws2mqtt/bridge_%08x/%s", DEVICE_ID, sensor);
	doc["stat_t"] = stattopic;

	char uniq_id[128];
	sprintf(uniq_id, "ws2mqtt_bridge_sensor_%s", sensor);
	doc["uniq_id"] = uniq_id;

	char msg[500];
	serializeJson(doc, msg, 500);

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/sensor/ws2mqtt_bridge_%08x/%s/config", DEVICE_ID, sensor);
		Serial.println(topic);
		mqttClient.publish(topic, msg, true);
	}

}

void addBridgeDescription(JsonObject dev) {
	char devid[32];
	sprintf(devid, "ws2mqtt_bridge_%08x", DEVICE_ID);
	dev["mf"] = "Tho85";
	dev["mdl"] = "WS2MQTT bridge";
	dev["ids"] = devid;

	char devname[128];
	sprintf(devname, "WS2MQTT bridge %08x", DEVICE_ID);
	dev["name"] = devname;
}

void sendMQTTBridgeEvent() {
	char eventtopic[200];
	sprintf(eventtopic, "ws2mqtt/bridge_%08x/event", DEVICE_ID);
	if (mqttClient.connected())
		mqttClient.publish(eventtopic, "alarm");
}

void loopBridgeSensors () {
	
	if (millis() - lastSensorPush > BRIDGE_SENSOR_INTERVAL * 1000) {
		// IP Address
		sendMQTTBridgeSensor((char*)"ip_addr", (char*)WiFi.localIP().toString().c_str());

		// RSSI
		char rssi[5];
		sprintf(rssi, "%d", (int)WiFi.RSSI());
		sendMQTTBridgeSensor((char*)"rssi", (char*)rssi);

		// MAC Address
		byte mac[6]; 
		WiFi.macAddress(mac);
		char mac_char[20];
		sprintf(mac_char, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		sendMQTTBridgeSensor((char*)"mac_addr", (char*)mac_char);

		// SSID
		sendMQTTBridgeSensor((char*)"ssid", (char*)WiFi.SSID().c_str());

		// Uptime
		char uptime[5];
		sprintf(uptime, "%d", esp_timer_get_time()/1000000);
		sendMQTTBridgeSensor((char*)"esp_uptime", (char*)uptime);

		esp_reset_reason_t reason = esp_reset_reason();
		char reason_str[20];
		switch (reason)
		{
		case ESP_RST_POWERON:
			sprintf(reason_str, "%s", "Power On");
			break;
		case ESP_RST_SW:
			sprintf(reason_str, "%s", "Software");
			break;
		case ESP_RST_PANIC:
			sprintf(reason_str, "%s", "Exception/Panic");
			break;
		case ESP_RST_INT_WDT:
			sprintf(reason_str, "%s", "Interupt Watchdog");
			break;
		case ESP_RST_TASK_WDT:
			sprintf(reason_str, "%s", "Task Watchdog");
			break;
		case ESP_RST_WDT:
			sprintf(reason_str, "%s", "Other Watchdog");
			break;
		case ESP_RST_DEEPSLEEP:
			sprintf(reason_str, "%s", "Exit Deep Sleep");
			break;
		case ESP_RST_BROWNOUT:
			sprintf(reason_str, "%s", "Brownout");
			break;
		
		default:
			sprintf(reason_str, "%s", "Uknown");
			break;
		}
		sendMQTTBridgeSensor((char*)"esp_reset_reason", (char*)reason_str);

		// ESP Chip
		sendMQTTBridgeSensor((char*)"esp_model", (char*)ESP.getChipModel());

		lastSensorPush = millis();
	}
}

void sendMQTTBridgeSensor (char* sensor, char* sensor_value) {

	if (mqttClient.connected()) {
		
		char init_topic[200];
		sprintf(init_topic, "ws2mqtt/bridge_%08x/%s", DEVICE_ID, sensor);
		Serial.println(init_topic);
		mqttClient.publish(init_topic, sensor_value, true);
	}
}

void announceMQTTDevice(device_t device) {
	switch(device.device_type) {
	case DEVICE_TYPE_CO:
		announceMQTTBinarySensor(device, (char *)"carbon_monoxide", (char *)"CO detector", (char *)"alarm", false);
		break;
	case DEVICE_TYPE_SMOKE:
		announceMQTTBinarySensor(device, (char *)"smoke", (char *)"Smoke detector", (char *)"alarm", false);
		break;
	default:
		announceMQTTBinarySensor(device, (char *)"safety", (char *)"Detector", (char *)"alarm", false);
		break;
	}
	announceMQTTBinarySensor(device, (char *)"battery", (char *)"Battery", (char *)"sensor_battery", true);
	announceMQTTBinarySensor(device, (char *)"battery", (char *)"Radio module battery", (char *)"radio_module_battery", true);
	announceMQTTBinarySensor(device, (char *)"plug", (char *)"Docked", (char *)"docked", true);
	announceMQTTBinarySensor(device, (char *)"problem", (char *)"Generic error", (char *)"generic_error", true);

	announceMQTTButtonEvent(device);
}

void addDeviceDescription(JsonObject dev, device_t device) {
	char devid[16];
	sprintf(devid, "ws2mqtt_%d", device.device_id);
	dev["ids"] = devid;

	char* manufacturer = (char *)"FireAngel";
	char* model = modelString(device.model);
	dev["mf"] = manufacturer;
	dev["mdl"] = model;

	char bridgeid[32];
	sprintf(bridgeid, "ws2mqtt_bridge_%08x", DEVICE_ID);
	dev["via_device"] = bridgeid;

	char devname[128];
	sprintf(devname, "%s %s %d", manufacturer, model, device.device_id);
	dev["name"] = devname;
}

void announceMQTTBinarySensor(
	device_t device,
	char* device_class,
	char* name,
	char* short_name,
	bool diagnostic
) {
	StaticJsonDocument<500> doc;

	char avtytopic[200];
	sprintf(avtytopic, "ws2mqtt/bridge_%08x/state", DEVICE_ID);
	doc["avty_t"] = avtytopic;

	JsonObject dev = doc.createNestedObject("dev");
	addDeviceDescription(dev, device);

	char sensor_name[128];
	char* model = modelString(device.model);
	sprintf(sensor_name, "%s %d %s", model, device.device_id, name);
	doc["name"] = sensor_name;

	doc["dev_cla"] = device_class;
	if (diagnostic)
		doc["entity_category"] = "diagnostic";

	char stat_t[128];
	sprintf(stat_t, "ws2mqtt/%d/%s/state", device.device_id, short_name);
	doc["stat_t"] = stat_t;

	char uniq_id[128];
	sprintf(uniq_id, "%d_%s_ws2mqtt", device.device_id, short_name);
	doc["uniq_id"] = uniq_id;

	char msg[500];
	serializeJson(doc, msg, 500);

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/binary_sensor/ws2mqtt_%d/%s/config", device.device_id, short_name);
		Serial.println(topic);
		mqttClient.publish(topic, msg, true);
	}
}

void sendMQTTState(uint32_t device_id, char* short_name, bool state) {
	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, "ws2mqtt/%d/%s/state", device_id, short_name);
		mqttClient.publish(topic, state ? "ON" : "OFF");
	}
}

void sendMQTTDeviceState(device_state_t ds) {
	sendMQTTState(ds.device_id, (char*)"generic_error", ds.generic_error);
	sendMQTTState(ds.device_id, (char*)"docked", ds.docked);
	sendMQTTState(ds.device_id, (char*)"sensor_battery", ds.sensor_battery);
	sendMQTTState(ds.device_id, (char*)"radio_module_battery", ds.radio_module_battery);
}

void announceMQTTButtonEvent(device_t device) {
	StaticJsonDocument<500> doc;

	// Create device
	JsonObject dev = doc.createNestedObject("dev");
	addDeviceDescription(dev, device);

	doc["atype"] = "trigger";
	doc["type"] = "button_short_press";
	doc["subtype"] = "button_1";

	char t[128];
	sprintf(t, "ws2mqtt/%d/event", device.device_id);
	doc["t"] = t;
	doc["pl"] = "test_button";

	char msg[500];
	serializeJson(doc, msg, 500);

	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/device_automation/ws2mqtt_%d/test_button/config", device.device_id);
		mqttClient.publish(topic, msg, true);
	}
}

void sendMQTTButtonEvent(uint32_t device_id) {
	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, "ws2mqtt/%d/event", device_id);
		mqttClient.publish(topic, "test_button");
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
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/binary_sensor/ws2mqtt_%d/%s/config", device.device_id, short_name);
		Serial.println(topic);
		mqttClient.publish(topic, NULL, true);
	} 
	
}

void removeMQTTButtonEvent(device_t device) {
	if (mqttClient.connected()) {
		char topic[200];
		sprintf(topic, HA_AUTODISCOVERY_PREFIX"/device_automation/ws2mqtt_%d/test_button/config", device.device_id);
		mqttClient.publish(topic, NULL, true);
	}
}

void handleMQTTCommand(char* command) {
	if (strncmp(command, "test_devices", 12) == 0) {
		sendTestButtonMsg(DEVICE_TYPE_ALL);
	} 
	else if (strncmp(command, "test_smoke", 10) == 0) { 
		sendTestButtonMsg(DEVICE_TYPE_SMOKE);
	} 
	else if (strncmp(command, "test_co", 7) == 0) { 
		sendTestButtonMsg(DEVICE_TYPE_CO);
	} 
#ifdef ENABLE_SILENCE_ALARM
	else if (strncmp(command, "silence_smoke", 13) == 0) {
		sendSilenceButtonMsg(DEVICE_TYPE_SMOKE);
	} 
	else if (strncmp(command, "silence_co", 10) == 0) {
		sendSilenceButtonMsg(DEVICE_TYPE_CO);
	}
#endif
#ifdef ENABLE_TRIGGER_ALARM
	else if (strncmp(command, "emergency_smoke", 15) == 0) {
		sendEmergencyButtonMsg(DEVICE_TYPE_SMOKE);
	}
	else if (strncmp(command, "emergency_co", 12) == 0) {
		sendEmergencyButtonMsg(DEVICE_TYPE_CO);
	}
#endif
	else if (strncmp(command, "esp_restart", 11) ==0) {
		ESP.restart();
	}

}

char* modelString(uint16_t model_id) {
	switch(model_id) {
	case MODEL_ST630DE: return (char *)"ST-630-DE";
	case MODEL_FP2620W2: return (char *)"FP2620W2";
	case MODEL_WST630: return (char *)"WST630";
	case MODEL_W2CO10X: return (char *)"W2-CO-10X";
	case MODEL_W2SVP630: return (char *)"W2-SVP-630";
	case MODEL_FP1720W2R: return (char *)"FP1720W2-R";
	default: return (char *)"Unkown";
	}
}
