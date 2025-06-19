// include/memory_monitor.h
#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Llenar un objeto JSON con información de memoria
void append_memory_info(JsonDocument &doc);

#endif // MEMORY_MONITOR_H
