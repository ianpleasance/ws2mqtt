#include "Arduino.h"
#include "device_db.h"

#include <sqlite3.h>
#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"

#define FORMAT_IF_FAILED true

sqlite3 *deviceDB;

const char* data = "Callback function called";
static int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	Serial.printf("%s: ", (const char*)data);
	for (i = 0; i<argc; i++){
		Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	Serial.printf("\n");
	return 0;
}

int db_open(const char *filename, sqlite3 **db) {
	int rc = sqlite3_open(filename, db);
	if (rc) {
		Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
		return rc;
	} else {
		Serial.printf("Opened database successfully\n");
	}
	return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
	if (db == NULL) {
		Serial.println("Database not open, cannot run SQL");
		return SQLITE_ERROR;
	}

	Serial.println(sql);

	int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if (rc != SQLITE_OK) {
		Serial.printf("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	return rc;
}


static int callbackGetDeviceFromDB(void *data, int argc, char **argv, char **azColName) {
	device_t* device_p = (device_t*)data;
	Serial.println("Found a matching device!");

	for (int i = 0; i < argc; i++){
		// Ignore NULL values
		if (argv[i] == NULL)
			continue;

		if (strcmp(azColName[i], "device_id") == 0)
			device_p->device_id = atoi(argv[i]);
		else if (strcmp(azColName[i], "device_type") == 0)
			device_p->device_type = atoi(argv[i]);
		else if (strcmp(azColName[i], "model") == 0)
			device_p->model = atoi(argv[i]);
		else if (strcmp(azColName[i], "sid") == 0)
			device_p->sid = atoi(argv[i]);
	}
	Serial.printf("\n");
	return 0;
}

int getDeviceFromDB(uint8_t sid, device_t* device) {
	char sql[128];
	int rc;

	if (deviceDB == NULL)
		return SQLITE_ERROR;

	snprintf(sql, sizeof(sql), "SELECT * FROM devices WHERE sid = %d LIMIT 1", (unsigned char)sid);
	rc = sqlite3_exec(deviceDB, sql, callbackGetDeviceFromDB, (void*)device, &zErrMsg);

	if (rc != SQLITE_OK) {
		Serial.printf("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	return rc;
}

static int callbackGetAllDevicesFromDB(void *data, int argc, char **argv, char **azColName) {
	device_t device;
	memset(&device, 0, sizeof(device));

	void (*callback)(device_t device) = (void(*)(device_t device)) data;

	for (int i = 0; i<argc; i++){
		// Ignore NULL values
		if (argv[i] == NULL)
			continue;

		if (strcmp(azColName[i], "device_id") == 0)
			device.device_id = atoi(argv[i]);
		else if (strcmp(azColName[i], "device_type") == 0)
			device.device_type = atoi(argv[i]);
		else if (strcmp(azColName[i], "model") == 0)
			device.model = atoi(argv[i]);
		else if (strcmp(azColName[i], "sid") == 0)
			device.sid = atoi(argv[i]);
	}

	callback(device);
	return 0;
}

static int callbackGetAllDeviceStatesFromDB(void *data, int argc, char **argv, char **azColName) {
	device_state_t ds;
	memset(&ds, 0, sizeof(ds));

	void (*callback)(device_state_t ds) = (void(*)(device_state_t ds)) data;

	Serial.println("Found a matching device state!");
	for (int i = 0; i<argc; i++){
		// Ignore NULL values
		if (argv[i] == NULL)
			continue;

		Serial.printf("%s = %s\n", azColName[i], argv[i]);

		if (strcmp(azColName[i], "device_id") == 0)
			ds.device_id = atoi(argv[i]);
		else if (strcmp(azColName[i], "sid") == 0)
			ds.sid = atoi(argv[i]);
		else if (strcmp(azColName[i], "generic_error") == 0)
			ds.generic_error = atoi(argv[i]);
		else if (strcmp(azColName[i], "docked") == 0)
			ds.docked = atoi(argv[i]);
		else if (strcmp(azColName[i], "sensor_battery_error") == 0)
			ds.sensor_battery = atoi(argv[i]);
		else if (strcmp(azColName[i], "radio_module_battery_error") == 0)
			ds.radio_module_battery = atoi(argv[i]);
	}

	callback(ds);
	return 0;
}

int getAllDevicesFromDB(void (*callback)(device_t device)) {
	int rc;

	if (deviceDB == NULL)
		return SQLITE_ERROR;

	sqlite3_initialize();

	rc = sqlite3_exec(deviceDB, "SELECT * FROM devices WHERE device_type IS NOT NULL;", callbackGetAllDevicesFromDB, (void*)callback, &zErrMsg);

	if (rc != SQLITE_OK) {
		Serial.printf("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	return rc;
}

int getAllDeviceStatesFromDB(void (*callback)(device_state_t ds)) {
	int rc;

	if (deviceDB == NULL)
		return SQLITE_ERROR;

	sqlite3_initialize();

	rc = sqlite3_exec(deviceDB, "SELECT * FROM devices WHERE generic_error IS NOT NULL;", callbackGetAllDeviceStatesFromDB, (void*)callback, &zErrMsg);

	if (rc != SQLITE_OK) {
		Serial.printf("SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	return rc;
}

void resetDeviceDB() {
	Serial.println("Resetting device DB");

	if (deviceDB == NULL)
		return;

	sqlite3_db_config(deviceDB, SQLITE_DBCONFIG_RESET_DATABASE, 1, 0);
	sqlite3_exec(deviceDB, "VACUUM", 0, 0, 0);
	sqlite3_db_config(deviceDB, SQLITE_DBCONFIG_RESET_DATABASE, 0, 0);

	migrateDeviceDB();
}

void setupDeviceDB() {
	connectDeviceDB();
	migrateDeviceDB();
}

void connectDeviceDB() {
	if (!SPIFFS.begin(FORMAT_IF_FAILED)) {
		Serial.println("Could not mount SPIFFS");
		return;
	}

	File root = SPIFFS.open("/");

	if (!root) {
		Serial.println("Could not list root");
		return;
	}

	if (!root.isDirectory()) {
		Serial.println("Root is not a directory");
		return;
	}

	sqlite3_initialize();

	if (db_open("/spiffs/devices.db", &deviceDB)) {
		// sqlite3_open hands back a handle even on failure; release it so
		// the NULL checks elsewhere work
		sqlite3_close(deviceDB);
		deviceDB = NULL;
	}
}

void migrateDeviceDB() {
	const char* schema = "CREATE TABLE IF NOT EXISTS devices (sid INTEGER NOT NULL PRIMARY KEY, device_id INTEGER, model INTEGER, device_type INTEGER, generic_error BOOLEAN, docked BOOLEAN, sensor_battery_error BOOLEAN, radio_module_battery_error BOOLEAN);";
	db_exec(deviceDB, schema);
}

void updateDeviceDBState(device_state_t ds) {
	char sql[400];
	snprintf(sql, sizeof(sql), "INSERT OR IGNORE INTO devices (sid) VALUES (%d)", ds.sid);
	db_exec(deviceDB, sql);

	snprintf(sql, sizeof(sql), "UPDATE devices SET device_id=%d, generic_error=%d, docked=%d, sensor_battery_error=%d, radio_module_battery_error=%d WHERE sid = %d", (int)ds.device_id, ds.generic_error, ds.docked, ds.sensor_battery, ds.radio_module_battery, ds.sid);
	db_exec(deviceDB, sql);
}

void insertDeviceDB(device_t device) {
	char sql[400];
	snprintf(sql, sizeof(sql), "INSERT OR IGNORE INTO devices (sid) VALUES (%d)", device.sid);
	db_exec(deviceDB, sql);

	snprintf(sql, sizeof(sql), "UPDATE devices SET device_id=%d, model=%d, device_type=%d WHERE sid = %d;", (int)device.device_id, device.model, device.device_type, device.sid);
	db_exec(deviceDB, sql);
}
