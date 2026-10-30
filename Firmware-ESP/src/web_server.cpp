#include "Arduino.h"
#include <WiFi.h>
#include <WebServer.h>

#include "web_server.h"
#include "web_page.h"
#include "web_data.h"
#include "device_manage.h"
#include "wisafe2_tx.h"

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

static void handleFavicon() {
	server.sendHeader("Cache-Control", "public, max-age=86400");
	server.send_P(200, "image/svg+xml", WEB_FAVICON_SVG);
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

static void handleNotFound() {
	server.send(404, "text/plain", "Not found");
}

// Write requests must carry a custom header. A web page on another site cannot
// add one without a CORS preflight, which this server does not answer, so this
// stops other sites from making a browser on your network change anything.
static bool writeAllowed() {
	if (server.header("X-Requested-With") != "ws2mqtt") {
		server.send(403, "application/json", "{\"ok\":false,\"error\":\"forbidden\"}");
		return false;
	}
	return true;
}

static void handleForget() {
	if (!writeAllowed())
		return;
	String arg = server.arg("sid");
	int sid = arg.toInt();
	if (arg.length() == 0 || sid < 0 || sid > 63) {
		server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad sid\"}");
		return;
	}
	if (!forgetDevice((uint8_t)sid)) {
		server.send(404, "application/json", "{\"ok\":false,\"error\":\"no such device\"}");
		return;
	}
	sendQuerySIDMap();
	server.send(200, "application/json", "{\"ok\":true}");
}

static void handleForgetAll() {
	if (!writeAllowed())
		return;
	int count = forgetAllDevices();
	char body[48];
	snprintf(body, sizeof(body), "{\"ok\":true,\"forgotten\":%d}", count);
	server.send(200, "application/json", body);
}

void setupWeb() {
	static const char* headerKeys[] = {"X-Requested-With"};
	server.collectHeaders(headerKeys, 1);

	server.on("/", HTTP_GET, handleRoot);
	server.on("/data/system.json", HTTP_GET, handleSystem);
	server.on("/data/module.json", HTTP_GET, handleModule);
	server.on("/data/devices.json", HTTP_GET, handleDevices);
	server.on("/data/mqtt.json", HTTP_GET, handleMqtt);
	server.on("/api/forget", HTTP_POST, handleForget);
	server.on("/api/forget-all", HTTP_POST, handleForgetAll);
	server.on("/favicon.svg", HTTP_GET, handleFavicon);
	server.on("/favicon.ico", HTTP_GET, handleFavicon);
	server.onNotFound(handleNotFound);
	server.begin();

	Serial.println("Web interface started on port 80");
}

void loopWeb() {
	server.handleClient();
}
