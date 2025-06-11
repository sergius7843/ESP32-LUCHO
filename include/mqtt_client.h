#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>

void mqtt_setup();
void mqtt_loop();
void mqtt_publish(const char* topic, const String& payload);
bool mqtt_is_connected();

#endif
