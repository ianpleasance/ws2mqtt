#include "Arduino.h"
#include "module_info.h"

module_info_t moduleInfo = {};

void noteUnrecognisedPacket(const uint8_t* msg, int length, bool withSub) {
	if (msg == NULL || length < 1)
		return;
	if (withSub && length < 2)
		withSub = false;

	moduleInfo.rxUnrecognised++;

	// Find this type, or add it if there is room. When the table is full the
	// packet is only counted in the total.
	int idx = -1;
	for (int i = 0; i < moduleInfo.unrecognisedUsed; i++) {
		unrecognised_t* u = &moduleInfo.unrecognised[i];
		if (u->type == msg[0] && u->hasSub == withSub && (!withSub || u->sub == msg[1])) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		if (moduleInfo.unrecognisedUsed >= UNRECOGNISED_TYPES_MAX)
			return;
		idx = moduleInfo.unrecognisedUsed++;
		unrecognised_t* u = &moduleInfo.unrecognised[idx];
		memset(u, 0, sizeof(*u));
		u->type = msg[0];
		u->hasSub = withSub;
		u->sub = withSub ? msg[1] : 0;
	}

	unrecognised_t* u = &moduleInfo.unrecognised[idx];
	u->count++;
	u->lastMs = millis();
	u->length = length < UNRECOGNISED_BYTES_MAX ? length : UNRECOGNISED_BYTES_MAX;
	memcpy(u->bytes, msg, u->length);
	moduleInfo.unrecognisedLast = idx;
}
