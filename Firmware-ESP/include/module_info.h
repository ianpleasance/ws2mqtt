#ifndef MODULE_INFO_H
#define MODULE_INFO_H 1

#include <stdint.h>

// Packets from the module that this firmware does not understand. One entry per
// distinct type (for 0xD4 diagnostic details the sub type counts as well).
#define UNRECOGNISED_TYPES_MAX 8
#define UNRECOGNISED_BYTES_MAX 12
typedef struct {
	uint8_t type;       // first byte of the packet
	bool hasSub;        // sub type is part of the identity (0xD4 packets)
	uint8_t sub;
	uint32_t count;
	uint32_t lastMs;
	uint8_t length;     // bytes kept in "bytes"
	uint8_t bytes[UNRECOGNISED_BYTES_MAX];  // start of the most recent packet of this type
} unrecognised_t;

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

	// Packets that matched no handler
	uint32_t rxUnrecognised;
	unrecognised_t unrecognised[UNRECOGNISED_TYPES_MAX];
	uint8_t unrecognisedUsed;
	uint8_t unrecognisedLast;  // index of the most recent type (valid when unrecognisedUsed > 0)
} module_info_t;

// Record a packet that no handler understood. withSub: the second byte is a
// sub type that is part of the identity (0xD4 details).
void noteUnrecognisedPacket(const uint8_t* msg, int length, bool withSub);

extern module_info_t moduleInfo;

// The radio module is polled every minute, so a module that has sent nothing
// at all for this long is not answering (dead, or the UART is disconnected).
// Measured from boot until the first frame arrives.
#define MODULE_RESPONSE_TIMEOUT_MS (10UL * 60UL * 1000UL)
bool moduleResponding();

#endif // MODULE_INFO_H
