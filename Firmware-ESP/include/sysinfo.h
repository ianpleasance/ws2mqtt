#ifndef SYSINFO_H
#define SYSINFO_H 1

#include "Arduino.h"

// Text for the last ESP32 reset reason
const char* resetReasonString(esp_reset_reason_t reason);

// Seconds since boot
uint32_t uptimeSeconds();

// Build date and time of the firmware
const char* buildTimestamp();

// Text for a PubSubClient state code (mqttClient.state())
const char* mqttStateString(int state);

// MQTT connection statistics, maintained by wifi.cpp and mqtt_log.cpp
typedef struct {
	uint32_t connectAttempts;
	uint32_t connects;
	uint32_t lastConnectMs;
	int lastFailState;
	uint32_t published;
	uint32_t publishFailed;
	uint32_t received;
} mqtt_stats_t;

extern mqtt_stats_t mqttStats;

#endif // SYSINFO_H
