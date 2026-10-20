#include "Arduino.h"

#include "spi_uart.h"
#include "wisafe2_rx.h"
#include "wisafe2_tx.h"
#include "wisafe2_packets.h"

#define DEBUG_SERIAL

// Maximum time (ms) to wait for the rest of a frame before giving up on it
#define RX_FRAME_TIMEOUT 200

bool binModeActivated = false;
uint8_t uartrxbuf[RXBUF_SIZE];
int uartrxbufPos = 0;
int uartrxbufExpectedLength = 0;
unsigned long lastRxByteAt = 0;

void setupSPI() {
	Serial2.setRxBufferSize(1024);
	Serial2.begin(19200, SERIAL_8N1, 16, 17);
}

void sendSPIMessage(uint8_t* msg) {
	int length;
	// Find the stop word, never reading past the end of the TX buffer
	for(length = 0; length < TXBUF_SIZE; length++)
		if (msg[length] == SPI_STOP_WORD)
			break;

	if (length == TXBUF_SIZE) {
		Serial.println("TX message has no stop word, not sending");
		return;
	}
	sendSPIMessage(msg, length+1);
}

void sendSPIMessage(uint8_t* msg, int length) {
	Serial2.write("}");
	Serial2.write(length);
	Serial2.write(msg, length);
}

void resetRXFrame() {
	binModeActivated = false;
	uartrxbufPos = 0;
	uartrxbufExpectedLength = 0;
}

void receiveSPIMessage(void (*rxCallback)(uint8_t*, int)) {
	// Drop a half received frame so a lost byte cannot wedge the parser
	if (binModeActivated && millis() - lastRxByteAt > RX_FRAME_TIMEOUT)
		resetRXFrame();

	while(Serial2.available()) {
		uint8_t c = (uint8_t)Serial2.read();
		lastRxByteAt = millis();
#ifdef DEBUG_SERIAL
		if (binModeActivated)
			// Do not spam console with binary data
			Serial.print('%');
		else
			Serial.print((char)c);
#endif

		if (!binModeActivated) {
			// Outside of a frame only the start marker is meaningful.
			// Inside a frame '{' (0x7B) is just a data byte.
			if (c == '{') {
				binModeActivated = true;
				uartrxbufPos = 0;
				uartrxbufExpectedLength = 0;
			}
		} else if (uartrxbufExpectedLength == 0) {
			if (c == 0 || c > RXBUF_SIZE) {
				// Invalid length, resynchronise
				resetRXFrame();
			} else {
				uartrxbufExpectedLength = c;
			}
		} else {
			uartrxbuf[uartrxbufPos++] = c;

			if (uartrxbufPos == uartrxbufExpectedLength) {
#ifdef DEBUG_SERIAL
				Serial.println();
#endif
				int length = uartrxbufPos;
				resetRXFrame();
				rxCallback(uartrxbuf, length);
			}
		}
	}

#ifdef DEBUG_SERIAL
	if(Serial.available())
		Serial2.print((char)Serial.read());
#endif
}
