#include "Arduino.h"
#include "status_query.h"
#include "config.h"
#include "wisafe2_packets.h"
#include "wisafe2_tx.h"
#include "device_runtime.h"

#define QUERY_QUEUE_SIZE 24

typedef struct {
	uint8_t sid;
	uint8_t sub;
} query_t;

static query_t queued[QUERY_QUEUE_SIZE];
static int queuedCount = 0;

static bool inFlight = false;
static query_t current;
static unsigned long sentAt = 0;

static bool isStatus(uint8_t sub) {
	return sub == SPI_TX_DIAGNOSTIC_DETAILS_STATUS;
}

void queryRequest(uint8_t sid, uint8_t sub) {
	if (inFlight && current.sid == sid && current.sub == sub)
		return;
	for (int i = 0; i < queuedCount; i++)
		if (queued[i].sid == sid && queued[i].sub == sub)
			return;
	if (queuedCount >= QUERY_QUEUE_SIZE) {
		Serial.println("Query queue full, dropping query");
		return;
	}
	queued[queuedCount].sid = sid;
	queued[queuedCount].sub = sub;
	queuedCount++;
}

void queryStatusIfUnknown(uint32_t device_id) {
#ifdef QUERY_STATUS_FOR_KNOWN_DEVICES
	device_runtime_t* d = runtimeByDeviceId(device_id);
	if (d != NULL && !d->alarmKnown)
		queryRequest(d->sid, SPI_TX_DIAGNOSTIC_DETAILS_STATUS);
#else
	(void)device_id;
#endif
}

void queryForgetSid(uint8_t sid) {
	int kept = 0;
	for (int i = 0; i < queuedCount; i++)
		if (queued[i].sid != sid)
			queued[kept++] = queued[i];
	queuedCount = kept;
	if (inFlight && current.sid == sid)
		inFlight = false;
}

void queryAnswered(uint8_t sid, uint8_t sub) {
	if (isStatus(sub))
		runtimeQueryReply(sid);
	if (inFlight && current.sid == sid && current.sub == sub)
		inFlight = false;
}

void queryAcknowledged() {
	if (inFlight)
		Serial.printf("Module acknowledged query to SID %d\n", current.sid);
}

void loopQuery() {
	if (inFlight) {
		if (millis() - sentAt < QUERY_TIMEOUT_MS)
			return;
		Serial.printf("Query to SID %d timed out\n", current.sid);
		if (isStatus(current.sub))
			runtimeQueryNoAnswer(current.sid);
		inFlight = false;
	}

	// Keep the module's own queue short so a query is not stuck behind others
	if (queuedCount == 0 || txQueueDepth() > 1)
		return;

	current = queued[0];
	memmove(&queued[0], &queued[1], (queuedCount - 1) * sizeof(query_t));
	queuedCount--;

	if (isStatus(current.sub))
		runtimeQuerySent(current.sid);
	sendQueryDiagnosticDetails(current.sid, current.sub);
	sentAt = millis();
	inFlight = true;
}
