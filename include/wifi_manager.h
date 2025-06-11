#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <string>

// Inicializa la conexión WiFi (llamar una vez desde setup o una tarea)
void wifi_init();

// Devuelve true si estamos conectados al WiFi
bool wifi_is_connected();

// Devuelve el SSID actual
std::string wifi_get_ssid();

// Devuelve la dirección IP local
std::string wifi_get_ip();

// Devuelve la dirección MAC
std::string wifi_get_mac();

// Devuelve el nivel de señal (RSSI)
int wifi_get_rssi();

#endif // WIFI_MANAGER_H
