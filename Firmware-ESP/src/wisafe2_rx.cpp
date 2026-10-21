#include "Arduino.h"

#include "config.h"
#include "wisafe2_packets.h"
#include "wifi_inc.h"
#include "spi_uart.h"
#include "wisafe2_escape.h"
#include "wisafe2_rx.h"
#include "wisafe2_tx.h"
#include "device_db.h"
#include "device_runtime.h"
#include "module_info.h"

extern int ownSid;
extern PubSubClient mqttClient;

// Copies a received message into a packet struct. If the message is shorter
// than the struct the remainder is zero filled (instead of reading stale bytes
// from the RX buffer) and a warning is logged.
static void copyPacket(void* dst, size_t dstSize, const uint8_t* msg, int length, const char* name) {
	memset(dst, 0, dstSize);
	if (length < (int)dstSize) {
		Serial.printf("Warning: short %s packet (%d bytes, expected %d)\n", name, length, (int)dstSize);
		moduleInfo.rxShortPackets++;
		memcpy(dst, msg, length > 0 ? length : 0);
	} else {
		memcpy(dst, msg, dstSize);
	}
}

void handleAlarmEvent(uint8_t* msg, int length) {
	pkt_rx_event_alarm_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "alarm");

	Serial.printf("Alarm event received from device %u cmd %02x type %02x\n", (unsigned int)pkt.device_id, pkt.cmd, pkt.device_type);

	runtimeSetAlarm(pkt.device_id, true);
	sendMQTTState(pkt.device_id, (char *)"alarm", true);
	sendMQTTBridgeEvent();
}

void handleAlarmOffEvent(uint8_t* msg, int length) {
	pkt_rx_event_alarm_off_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "alarm off");

	Serial.printf("Alarm off event received from device %u cmd %02x type %02x\n", (unsigned int)pkt.device_id, pkt.cmd, pkt.device_type);

	runtimeSetAlarm(pkt.device_id, false);
	sendMQTTState(pkt.device_id, (char *)"alarm", false);
}

void handleButtonEvent(uint8_t* msg, int length) {
	pkt_rx_event_button_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "button");

	Serial.printf("Button press received from device %u cmd %02x\n", (unsigned int)pkt.device_id, pkt.cmd);

	runtimeSetButton(pkt.device_id);
	sendMQTTButtonEvent(pkt.device_id);
	sendMQTTBridgeEvent();
}

void handleError(uint8_t* msg, int length) {
	pkt_rx_error_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "error");

	Serial.printf("Error received from device %u cmd %02x error %02x\n", (unsigned int)pkt.device_id, pkt.cmd, pkt.error_flags);

	device_state_t ds;
	ds.sid = pkt.sid;
	ds.device_id = pkt.device_id;
	ds.generic_error = (pkt.error_flags & SPI_RX_ERROR_FLAG_GENERIC);
	ds.docked = (pkt.error_flags & SPI_RX_ERROR_FLAG_DOCKED);
	ds.sensor_battery = (pkt.error_flags & SPI_RX_ERROR_FLAG_SENSOR_BATTERY);
	ds.radio_module_battery = (pkt.error_flags & SPI_RX_ERROR_FLAG_RADIO_MODULE_BATTERY);

	sendMQTTDeviceState(ds);
	updateDeviceDBState(ds);
	runtimeSetState(ds);
}

void handleUpdateSIDMapResponse(uint8_t* msg, int length) {
	pkt_rx_diagnostic_details_update_sidmap_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "SID map");

	uint64_t sidmap = pkt.sidmap;
	moduleInfo.sidmapReceived = true;
	moduleInfo.sidmapAtMs = millis();
	moduleInfo.sidmap = sidmap;
	Serial.printf("Update SID Map received. SID map: %08x%08x\n", (uint32_t)(sidmap >> 32), (uint32_t)sidmap);

	for (uint8_t sid = 0; sid < 64; sid++) {
		if ((sidmap >> sid) & 0x01) {
			Serial.printf(" Found SID %d\n", sid);
			checkIfDeviceKnown(sid);
		}
	}
}

void checkIfDeviceKnown(uint8_t sid) {
	if (sid == ownSid)
		return;

	device_t device;
	memset(&device, 0, sizeof(device));

	// Check if device is in cache
	getDeviceFromDB(sid, &device);
	if (device.device_type == 0x00) {
		Serial.println("Device not in DB, querying");
		sendQueryDiagnosticDetails(sid, SPI_TX_DIAGNOSTIC_DETAILS_ID);
	} else {
		Serial.printf("Device found, device_id: %u\n", (unsigned int)device.device_id);
#ifdef QUERY_STATUS_FOR_KNOWN_DEVICES
		// The alarm state is only learned from an alarm message or a status
		// reply. After a reboot (or a lost reply) ask the device for it.
		device_runtime_t* rt = runtimeBySid(sid);
		if (rt == NULL || !rt->alarmKnown) {
			Serial.println("Status not known, querying");
			sendQueryDiagnosticDetails(sid, SPI_TX_DIAGNOSTIC_DETAILS_STATUS);
		}
#endif
	}
}

// Response to D3 06 xx 00 7E
// => D4 06 xx ...
void handleUpdateRemoteStatusResponse(uint8_t* msg, int length) {
	pkt_rx_diagnostic_details_remote_status_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "remote status");

	Serial.printf("Received diagnostic response for: %u\n", (unsigned int)pkt.device_id);

	// Set initial state for sensor. There is no way to query for a
	// currently ongoing alarm, so always assume alarm is off
	runtimeSetDiag(pkt.sid, pkt.device_id, pkt.battery1, pkt.battery2, pkt.rssi,
		pkt.firmware_version, pkt.flags, pkt.radio_fault_count);
	runtimeSetAlarm(pkt.device_id, false);
	sendMQTTState(pkt.device_id, (char *)"alarm", false);
}

// Response to D3 06 xx 01 7E
void handleDiagnosticDetailsRemoteIdResponse(uint8_t* msg, int length) {
	pkt_rx_diagnostic_details_remote_id_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "remote id");

	Serial.printf("Received remote id for: %u\n", (unsigned int)pkt.device_id);

	device_t device;
	memset(&device, 0, sizeof(device));
	device.device_id = pkt.device_id;
	device.model = pkt.model;
	device.device_type = pkt.device_type;
	device.sid = pkt.sid;

	announceMQTTDevice(device);
	insertDeviceDB(device);
	runtimeSetInfo(device);

	sendQueryDiagnosticDetails(pkt.sid, SPI_TX_DIAGNOSTIC_DETAILS_STATUS);
}

// Automatically sent when new device joins network
// => D4 09 xx ...
void handleNewDeviceResponse(uint8_t* msg, int length) {
	pkt_rx_diagnostic_details_new_device_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "new device");

	Serial.printf("New device announced: %u sid %d\n", (unsigned int)pkt.device_id, pkt.sid);
	// TODO: Better handle sid <-> device_id mismatch / conflict
	checkIfDeviceKnown(pkt.sid);
}

void handleDiagnosticResponse(uint8_t* msg, int length) {
	pkt_rx_diagnostic_response_t pkt;
	copyPacket(&pkt, sizeof(pkt), msg, length, "diagnostic");

	Serial.printf("Diagnostic response received\n");

	moduleInfo.diagReceived = true;
	moduleInfo.diagAtMs = millis();
	moduleInfo.sid = pkt.sid;
	moduleInfo.device_id = pkt.device_id;
	moduleInfo.battery1 = pkt.battery1;
	moduleInfo.battery2 = pkt.battery2;
	moduleInfo.rssi = pkt.rssi;
	moduleInfo.firmware_version = pkt.firmware_version;
	moduleInfo.flags = pkt.flags;
	moduleInfo.radio_fault_count = pkt.radio_fault_count;

	if (pkt.device_id == 0x00) {
		Serial.println("Module not initialized");
	}

	if (pkt.sid == 0x40) { // 64 -> higher than maximum SID -> device not connected?
		Serial.println("Module not connected");

		if (ownSid != 0x40) {
			// Device was connected before, so deannounce all
			// devices
			getAllDevicesFromDB(removeMQTTDevice);

			// Clear SQLite database
			resetDeviceDB();
			runtimeClear();
		}
	}
	Serial.printf("Setting own sid to %d\n", pkt.sid);
	ownSid = pkt.sid;
}

void handleDiagnosticDetailsResponse(uint8_t* msg, int length) {
	if (length < 2)
		return;

	switch(msg[1]) {
	case SPI_DIAGNOSTIC_DETAILS_UPDATE_SIDMAP:
		handleUpdateSIDMapResponse(msg, length);
		break;
	case SPI_DIAGNOSTIC_DETAILS_REMOTE_STATUS:
		handleUpdateRemoteStatusResponse(msg, length);
		break;
	case SPI_DIAGNOSTIC_DETAILS_NEW_DEVICE:
		handleNewDeviceResponse(msg, length);
		break;
	}
}

void handleRX(uint8_t* msg, int length) {
	length = unescapeMessage((uint8_t*)msg, length);
	if (length < 1)
		return;

	switch(msg[0]) {
	case SPI_RX_REQUEST_WELCOME:
		moduleInfo.welcomeCount++;
		moduleInfo.lastWelcomeMs = millis();
		sendWelcomeMsg();
		break;
	case SPI_RX_EVENT_ALARM:
		handleAlarmEvent(msg, length);
		break;
	case SPI_RX_EVENT_ALARM_OFF:
		handleAlarmOffEvent(msg, length);
		break;
	case SPI_RX_EVENT_BUTTON:
		handleButtonEvent(msg, length);
		break;
	case SPI_RX_ERROR:
		handleError(msg, length);
		break;
	case SPI_RX_DIAGNOSTIC_RESPONSE:
		handleDiagnosticResponse(msg, length);
		break;
	case SPI_RX_DIAGNOSTIC_DETAILS_RESPONSE:
		handleDiagnosticDetailsResponse(msg, length);
		break;
	case SPI_RX_DIAGNOSTIC_DETAILS_REMOTE_ID:
		handleDiagnosticDetailsRemoteIdResponse(msg, length);
		break;
	}
}
