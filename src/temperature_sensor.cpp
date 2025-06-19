// src/temperature_sensor.cpp
#include "temperature_sensor.h"
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

void temperatureTask(void *parameter) {
    analogReadResolution(ADC_RESOLUTION_BITS);
    while (true) {
        int raw = analogRead(TEMP_SENSOR_PIN);
        float voltage = (raw / float(ADC_MAX_VALUE)) * ADC_REF_VOLTAGE;
        float temperatureC = voltage * 100.0;  // LM35: 10mV/°C

        StaticJsonDocument<128> doc;
        doc["temperature_c"] = temperatureC;
        doc["adc"] = raw;
        doc["timestamp"] = millis();

        String json;
        serializeJsonPretty(doc, json);

        Serial.println("[" MQTT_TOPIC_TEMPERATURE "]");
        Serial.println(json);

        mqtt_publish(MQTT_TOPIC_TEMPERATURE, json);

        vTaskDelay(pdMS_TO_TICKS(TEMP_REPORT_INTERVAL));
    }
}

void start_temperature_task() {
    xTaskCreatePinnedToCore(
        temperatureTask,
        "TemperatureTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}
