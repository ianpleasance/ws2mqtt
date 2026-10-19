#ifndef CONFIG_H
#define CONFIG_H 1

// Device ID for Home Assistant. Change only if it conflicts with another
// Home Assistant device or you are running multiple ws2mqtt instances.
#define DEVICE_ID 0x317E33

// How often new devices should be queried, in seconds.
#define QUERY_NEW_DEVICES_INTERVAL 60

// Optional alarm controls from Home Assistant.
// Keep these commented out to disable the features.
// Only enable them after verifying that the corresponding packet definitions
// and command handlers are implemented and tested.
#define ENABLE_SILENCE_ALARM
#define ENABLE_TRIGGER_ALARM

#endif // CONFIG_H

