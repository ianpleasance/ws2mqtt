#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H 1

#include "ArduinoJson.h"
#include "wisafe2_packets.h"
#include "device_db.h"

#define HA_AUTODISCOVERY_PREFIX "homeassistant"

#define BRIDGE_SENSOR_INTERVAL 10 // seconds between sending bridge sensor updates

void announceAllDevices();
void publishAllCachedDeviceStates();

void announceMQTTBridge();
void announceMQTTBridgeEntities();
void announceMQTTBridgeButtonEntity(char* name, char* command, bool diagnostic, bool enabled, char* icon);
void announceMQTTBridgeSensorEntity(
	char* name, 
	char* sensor, 
	char* dev_class, 
	char* unit, 
	bool diagnostic, 
	bool enabled, 
	char* icon
);

void addBridgeDescription(JsonObject dev);
void sendMQTTBridgeEvent();

void loopBridgeSensors();
void sendMQTTBridgeSensor (char* sensor, char* sensor_value);

void announceMQTTDevice(device_t device);
void addDeviceDescription(JsonObject dev, device_t device);
void announceMQTTBinarySensor(
	device_t device,
	char* device_class,
	char* name,
	char* short_name,
	bool diagnostic
);
void sendMQTTState(uint32_t deviceid, char* short_name, bool state);
void sendMQTTDeviceState(device_state_t ds);
void announceMQTTButtonEvent(device_t device);
void sendMQTTButtonEvent(uint32_t deviceid);

void removeMQTTDevice(device_t device);
void removeMQTTBinarySensor(device_t device, char* short_name);
void removeMQTTButtonEvent(device_t device);

void handleMQTTCommand(char* command);

char* modelString(uint16_t model_id);
#endif
