#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H 1

#include "ArduinoJson.h"
#include "wisafe2_packets.h"
#include "device_db.h"
#include "device_models.h"

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
	bool diagnostic,
	bool enabled = true
);
void sendMQTTState(uint32_t deviceid, char* short_name, bool state);

// Location sensor (free text such as "Kitchen") for each device
void announceMQTTLocationSensor(device_t device);
void sendMQTTLocation(uint32_t deviceid, const char* location);
void sendMQTTDeviceState(device_state_t ds);
void announceMQTTButtonEvent(device_t device);

// Diagnostic sensors and a Probe button on each device
void announceMQTTDeviceSensors(device_t device);
void announceMQTTDeviceProbeButton(device_t device);
void sendMQTTDeviceSensors(uint32_t deviceid);
void sendMQTTDeviceSensor(uint32_t deviceid, const char* key);
void sendMQTTButtonEvent(uint32_t deviceid);

void removeMQTTDevice(device_t device);
void removeMQTTBinarySensor(device_t device, char* short_name);
void removeMQTTButtonEvent(device_t device);

void handleMQTTCommand(char* command);

#endif
