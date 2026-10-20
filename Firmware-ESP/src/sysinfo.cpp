#include "sysinfo.h"

mqtt_stats_t mqttStats = {};

const char* resetReasonString(esp_reset_reason_t reason) {
	switch (reason) {
	case ESP_RST_POWERON:  return "Power On";
	case ESP_RST_SW:       return "Software";
	case ESP_RST_PANIC:    return "Exception/Panic";
	case ESP_RST_INT_WDT:  return "Interrupt Watchdog";
	case ESP_RST_TASK_WDT: return "Task Watchdog";
	case ESP_RST_WDT:      return "Other Watchdog";
	case ESP_RST_DEEPSLEEP: return "Exit Deep Sleep";
	case ESP_RST_BROWNOUT: return "Brownout";
	default:               return "Unknown";
	}
}

uint32_t uptimeSeconds() {
	return (uint32_t)(esp_timer_get_time() / 1000000);
}

const char* buildTimestamp() {
	return __DATE__ " " __TIME__;
}

const char* mqttStateString(int state) {
	switch (state) {
	case -4: return "Connection timeout";
	case -3: return "Connection lost";
	case -2: return "Connect failed";
	case -1: return "Disconnected";
	case 0:  return "Connected";
	case 1:  return "Bad protocol";
	case 2:  return "Bad client ID";
	case 3:  return "Server unavailable";
	case 4:  return "Bad credentials";
	case 5:  return "Unauthorised";
	default: return "Unknown";
	}
}
