#include "status_reporter.h"
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

void statusReporterTask(void *parameter) {
    while (true) {
        StaticJsonDocument<256> doc;

        doc["online"] = mqtt_is_connected();
        doc["timestamp"] = millis();
        doc["client_id"] = MQTT_CLIENT_ID;

        String json;
        serializeJsonPretty(doc, json);

        Serial.println("[" MQTT_TOPIC_HEARTBEAT "]");
        Serial.println(json);

        mqtt_publish(MQTT_TOPIC_HEARTBEAT, json);

        vTaskDelay(pdMS_TO_TICKS(MQTT_HEARTBEAT_INTERVAL));
    }
}

void start_status_reporter_task() {
    xTaskCreatePinnedToCore(
        statusReporterTask,
        "StatusReporterTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}
