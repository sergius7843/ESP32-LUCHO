#include "wifi_manager.h"
#include "config.h"

void wifi_init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Conectando a WiFi");

    // Esperar conexión
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n Conectado a WiFi");
    } else {
        Serial.println("\n No se pudo conectar a WiFi");
    }
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

std::string wifi_get_ssid() {
    return WiFi.SSID().c_str();
}

std::string wifi_get_ip() {
    return WiFi.localIP().toString().c_str();
}

std::string wifi_get_mac() {
    return WiFi.macAddress().c_str();
}

int wifi_get_rssi() {
    return WiFi.RSSI();
}