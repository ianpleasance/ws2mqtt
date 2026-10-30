#ifndef STATUS_QUERY_H
#define STATUS_QUERY_H 1

#include <stdint.h>

// Remote queries (status 0x00 or identity 0x01) are sent to the radio module
// one at a time. The next one is sent when the previous one is answered or
// times out. The module's immediate acknowledgement (0x46) does not end a query.

// Time to wait for an answer before moving on
#define QUERY_TIMEOUT_MS 6000

// Queue a query. Duplicates of a query that is already waiting or in flight are ignored.
void queryRequest(uint8_t sid, uint8_t subsubtype);

// Queue a query to be sent next, ahead of anything already waiting (manual probe)
void queryRequestFirst(uint8_t sid, uint8_t subsubtype);

// Queue a status query for a device whose alarm state is not known yet
void queryStatusIfUnknown(uint32_t device_id);

// Drop any queued or in-flight query for this SID (the device was forgotten)
void queryForgetSid(uint8_t sid);

// A reply arrived for this SID / query type
void queryAnswered(uint8_t sid, uint8_t subsubtype);

// The module acknowledged the command (0x46 7E); the reply is still to come
void queryAcknowledged();

// Send the next queued query, or time out the one in flight. Call from loop().
void loopQuery();

#endif // STATUS_QUERY_H
