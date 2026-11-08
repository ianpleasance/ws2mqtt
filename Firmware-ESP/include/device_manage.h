#ifndef DEVICE_MANAGE_H
#define DEVICE_MANAGE_H 1

#include <stdint.h>

// Forget a device: removes it from Home Assistant (discovery and retained
// states), the database, the in-memory list and the query queue. A device that
// is still on the radio network is found again by the next SID map scan.
// The device's saved location is cleared too.
// Returns false if there is no device with that SID.
bool forgetDevice(uint8_t sid);

// Ask a device for its status now (ahead of any other waiting query).
// Returns false if there is no device with that SID.
bool probeDevice(uint8_t sid);

// Mark every device that can be docked (not the control units) as docked, in
// the database, the web interface and Home Assistant. This is an assumption made
// by the user, as the docked state cannot be read from a device; the next real
// report from a device replaces it. Other fault flags are left as they were.
// Returns how many devices were changed.
int markAllDocked();

// Ask every known device for its status. Returns how many were queued.
int probeAllDevices();

// Maximum length of a location, in bytes
#define LOCATION_MAX_LEN 32

// Set the location of the device with this SID (empty text clears it). The text
// is cleaned up (control characters removed, trimmed, cut to LOCATION_MAX_LEN).
// It is saved, and sent to Home Assistant. Returns false if there is no such device.
bool setDeviceLocation(uint8_t sid, const char* value);

// Forget every device. Saved locations are kept, so devices that are still on
// the radio network come back with theirs. Returns how many were forgotten.
int forgetAllDevices();

#endif // DEVICE_MANAGE_H
