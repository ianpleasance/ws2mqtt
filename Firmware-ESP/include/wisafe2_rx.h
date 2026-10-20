#ifndef WISAFE2_RX_H
#define WISAFE2_RX_H 1

#include <stdint.h>

#define RXBUF_SIZE 128

void handleAlarmEvent(uint8_t* msg, int length);
void handleAlarmOffEvent(uint8_t* msg, int length);
void handleButtonEvent(uint8_t* msg, int length);
void handleError(uint8_t* msg, int length);
void handleUpdateSIDMapResponse(uint8_t* msg, int length);
void checkIfDeviceKnown(uint8_t sid);
void handleUpdateRemoteStatusResponse(uint8_t* msg, int length);
void handleDiagnosticDetailsRemoteIdResponse(uint8_t* msg, int length);
void handleDiagnosticResponse(uint8_t* msg, int length);
void handleDiagnosticDetailsResponse(uint8_t* msg, int length);
void handleNewDeviceResponse(uint8_t* msg, int length);
void handleRX(uint8_t* msg, int length);
#endif
