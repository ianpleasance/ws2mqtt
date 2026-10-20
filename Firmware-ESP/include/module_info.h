#ifndef MODULE_INFO_H
#define MODULE_INFO_H 1

#include <stdint.h>

// Runtime counters and the last data received from the WiSafe2 radio module.
// Purely informational (web UI); nothing in the protocol logic depends on it.
typedef struct {
	// Last diagnostic response (0xD2) about the module itself
	bool diagReceived;
	uint32_t diagAtMs;
	uint8_t sid;
	uint32_t device_id;
	uint8_t battery1;
	uint8_t battery2;
	uint8_t rssi;
	uint8_t firmware_version;
	uint8_t flags;
	uint8_t radio_fault_count;

	// Last SID map (0xD4 03)
	bool sidmapReceived;
	uint32_t sidmapAtMs;
	uint64_t sidmap;

	// Welcome handshake (0x41 request, 0x91 reply)
	uint32_t welcomeCount;
	uint32_t lastWelcomeMs;

	// UART traffic
	uint32_t rxFrames;
	uint32_t rxFrameTimeouts;
	uint32_t rxBadLength;
	uint32_t rxShortPackets;
	uint32_t lastRxMs;
	uint32_t txSent;
	uint32_t txDropped;
	uint32_t lastTxMs;
} module_info_t;

extern module_info_t moduleInfo;

#endif // MODULE_INFO_H
