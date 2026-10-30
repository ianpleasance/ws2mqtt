#ifndef DEVICE_MANAGE_H
#define DEVICE_MANAGE_H 1

#include <stdint.h>

// Forget a device: removes it from Home Assistant (discovery and retained
// states), the database, the in-memory list and the query queue. A device that
// is still on the radio network is found again by the next SID map scan.
// Returns false if there is no device with that SID.
bool forgetDevice(uint8_t sid);

// Ask a device for its status now (ahead of any other waiting query).
// Returns false if there is no device with that SID.
bool probeDevice(uint8_t sid);

// Ask every known device for its status. Returns how many were queued.
int probeAllDevices();

// Forget every device. Returns how many were forgotten.
int forgetAllDevices();

#endif // DEVICE_MANAGE_H
