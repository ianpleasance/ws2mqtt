#ifndef WEB_SERVER_H
#define WEB_SERVER_H 1

// Web interface on port 80. Call setupWeb() once WiFi is up and loopWeb()
// from the main loop.
void setupWeb();
void loopWeb();

#endif // WEB_SERVER_H
