#include "Arduino.h"
#include <WiFi.h>
#include <WebServer.h>

#include "web_server.h"
#include "web_page.h"
#include "web_data.h"

static WebServer server(80);

// Print that sends everything written to it to the client as HTTP chunks, so
// that large JSON responses never need to be held in memory.
class ChunkPrint : public Print {
public:
	explicit ChunkPrint(WebServer& s) : web(s), len(0) {}

	size_t write(uint8_t c) override {
		buf[len++] = (char)c;
		if (len == sizeof(buf) - 1)
			flushChunk();
		return 1;
	}

	size_t write(const uint8_t* data, size_t size) override {
		for (size_t i = 0; i < size; i++)
			write(data[i]);
		return size;
	}

	void flushChunk() {
		if (len > 0) {
			buf[len] = '\0';
			web.sendContent(String(buf));
			len = 0;
		}
	}

private:
	WebServer& web;
	char buf[512];
	size_t len;
};

static void beginJson() {
	server.setContentLength(CONTENT_LENGTH_UNKNOWN);
	server.sendHeader("Cache-Control", "no-store");
	server.send(200, "application/json", "");
}

static void endJson(ChunkPrint& out) {
	out.flushChunk();
	server.sendContent("");  // terminates the chunked response
}

static void handleRoot() {
	server.sendHeader("Cache-Control", "no-cache");
	server.send_P(200, "text/html; charset=utf-8", WEB_INDEX_HTML);
}

static void handleSystem() {
	beginJson();
	ChunkPrint out(server);
	writeSystemJson(out);
	endJson(out);
}

static void handleModule() {
	beginJson();
	ChunkPrint out(server);
	writeModuleJson(out);
	endJson(out);
}

static void handleDevices() {
	beginJson();
	ChunkPrint out(server);
	writeDevicesJson(out);
	endJson(out);
}

static void handleMqtt() {
	bool all = server.hasArg("all") && server.arg("all") == "1";
	beginJson();
	ChunkPrint out(server);
	writeMqttJson(out, all);
	endJson(out);
}

static void handleNoContent() {
	server.send(204);
}

static void handleNotFound() {
	server.send(404, "text/plain", "Not found");
}

void setupWeb() {
	server.on("/", HTTP_GET, handleRoot);
	server.on("/data/system.json", HTTP_GET, handleSystem);
	server.on("/data/module.json", HTTP_GET, handleModule);
	server.on("/data/devices.json", HTTP_GET, handleDevices);
	server.on("/data/mqtt.json", HTTP_GET, handleMqtt);
	server.on("/favicon.ico", HTTP_GET, handleNoContent);
	server.onNotFound(handleNotFound);
	server.begin();

	Serial.println("Web interface started on port 80");
}

void loopWeb() {
	server.handleClient();
}
