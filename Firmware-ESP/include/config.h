#ifndef CONFIG_H
#define CONFIG_H 1

// Device ID for home assistant. Shouldn't need to change unless there is a 
// clash with another home assistant device or you need more than one ws2mqtt instance.
#define DEVICE_ID 0x317E33 

// How often should new devices be queried
#define QUERY_NEW_DEVICES_INTERVAL 60 // seconds

// Enables the ability to silence the alarm from Home Assistant. Comment out the below line to disable.
//#define ENABLE_SILENCE_ALARM

// Enables the ability to trigger an alarm from Home Assistant. Comment out the below line to disable.
//#define ENABLE_TRIGGER_ALARM

#endif
