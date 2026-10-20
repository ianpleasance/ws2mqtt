#ifndef MAIN_H
#define MAIN_H 1

// SID of this gateway on the WiSafe2 network, -1 until the module has told us.
// Defined in main.cpp
extern int ownSid;

void createDevice();
void updateDeviceState();
void buttonPressReceived();
void setup();
void loopUpdateSIDMap();
void loop();
#endif
