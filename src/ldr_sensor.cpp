#include "ldr_sensor.h"
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

static void ldrTask(void *parameter) {
    // Aseguramos resolución ADC igual que temperatura
    analogReadResolution(ADC_RESOLUTION_BITS);

    while (true) {
        int raw = analogRead(LDR_SENSOR_PIN);
        // Aquí podrías mapear raw a lux si tienes la ecuación del sensor
        // float lux = map(raw, 0, ADC_MAX_VALUE, 0, 1000); 

        StaticJsonDocument<128> doc;
        doc["ldr_raw"] = raw;
        // doc["lux"] = lux;
        doc["timestamp"] = millis();

        String json;
        serializeJsonPretty(doc, json);

        Serial.println("[" MQTT_TOPIC_LDR "]");
        Serial.println(json);

        mqtt_publish(MQTT_TOPIC_LDR, json);

        vTaskDelay(pdMS_TO_TICKS(LDR_REPORT_INTERVAL));
    }
}

void start_ldr_task() {
    xTaskCreatePinnedToCore(
        ldrTask,
        "LDRTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}
