#include "memory_monitor.h"
#include <SPIFFS.h>

void append_memory_info(JsonDocument &doc) {
    JsonObject memory = doc.createNestedObject("memory");

    // === RAM / HEAP ===
    uint32_t heap_total = ESP.getHeapSize();
    uint32_t heap_free = ESP.getFreeHeap();
    uint32_t heap_used = heap_total - heap_free;

    JsonObject heap = memory.createNestedObject("heap");
    heap["total"] = heap_total;
    heap["free"] = heap_free;
    heap["used"] = heap_used;

    // === Flash (SPIFFS) ===
    if (SPIFFS.begin(true)) {
        uint32_t flash_total = SPIFFS.totalBytes();
        uint32_t flash_used = SPIFFS.usedBytes();
        uint32_t flash_free = flash_total - flash_used;

        JsonObject flash = memory.createNestedObject("flash");
        flash["total"] = flash_total;
        flash["used"] = flash_used;
        flash["free"] = flash_free;
    }

    // === Uptime ===
    memory["uptime_ms"] = millis();
}
