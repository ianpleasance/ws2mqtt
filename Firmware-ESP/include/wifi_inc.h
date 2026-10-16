#ifndef WIFI_H
#define WIFI_H 1

#include "Arduino.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"
#include "homeassistant.h"
#include <ArduinoOTA.h>

boolean reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setupMQTT();
void loopMQTT();

void setupWifi();

void setupOTA();
void loopOTA();

#endif
