#include "json_formatter.h"
#include "wifi_manager.h"
#include "memory_monitor.h"
#include <ArduinoJson.h>

// Generar JSON solo con datos WiFi
String generate_wifi_json() {
    StaticJsonDocument<256> doc;

    doc["connected"] = wifi_is_connected();
    doc["ssid"] = wifi_get_ssid();
    doc["ip"] = wifi_get_ip();
    doc["mac"] = wifi_get_mac();
    doc["rssi"] = wifi_get_rssi();

    String output;
    serializeJsonPretty(doc, output);
    return output;
}

// Generar JSON solo con datos de memoria
String generate_memory_json() {
    StaticJsonDocument<512> doc;

    append_memory_info(doc);

    String output;
    serializeJsonPretty(doc, output);
    return output;
}
