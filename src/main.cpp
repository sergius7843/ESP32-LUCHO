#include <Arduino.h>
#include "wifi_manager.h"
#include "json_formatter.h"
#include "memory_monitor.h"
#include "mqtt_client.h"
#include "config.h"
#include "status_reporter.h"
#include "temperature_sensor.h"

// --- Incluir el controlador de luces del auditorio
#include "light_controller.h"

// Tarea que imprime y publica estado WiFi
void wifiInfoTask(void *parameter) {
    while (true) {
        String wifiJson = generate_wifi_json();
        Serial.println("[wifi/status]");
        Serial.println(wifiJson);
        mqtt_publish(MQTT_TOPIC_WIFI, wifiJson);
        vTaskDelay(pdMS_TO_TICKS(WIFI_INFO_INTERVAL));
    }
}

// Tarea que imprime y publica estado de memoria
void memoryInfoTask(void *parameter) {
    while (true) {
        String memJson = generate_memory_json();
        Serial.println("[system/memory]");
        Serial.println(memJson);
        mqtt_publish(MQTT_TOPIC_MEMORY, memJson);
        vTaskDelay(pdMS_TO_TICKS(WIFI_INFO_INTERVAL));
    }
}

// Tarea periódica para reportar estado de luces (opcional)
void lightsStatusTask(void *parameter) {
    while (true) {
        // Publicar estado de luces cada 30 segundos
        if (mqtt_is_connected()) {
            publish_all_lights_status();
        }
        vTaskDelay(pdMS_TO_TICKS(30000)); // 30 segundos
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("    ESP32 AUDITORIUM CONTROLLER");
    Serial.println("=================================");

    // Inicializar WiFi y MQTT
    wifi_init();    // Inicializa y conecta a WiFi
    mqtt_setup();   // Configura servidor y callback MQTT

    // ----- Inicializar el controlador de luces del auditorio -----
    Serial.println("[SETUP] Inicializando controlador de luces...");
    init_light_controller();

    // Crear tareas FreeRTOS
    Serial.println("[SETUP] Creando tareas FreeRTOS...");
    
    // Tarea WiFi
    xTaskCreatePinnedToCore(
        wifiInfoTask,
        "WiFiInfoTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    // Tarea memoria
    xTaskCreatePinnedToCore(
        memoryInfoTask,
        "MemoryInfoTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    // Tarea de status (heartbeat MQTT)
    start_status_reporter_task();

    // Tarea para el sensor de temperatura
    start_temperature_task();

    // Tarea periódica de estado de luces (opcional)
    xTaskCreatePinnedToCore(
        lightsStatusTask,
        "LightsStatusTask",
        4096,
        NULL,
        1,
        NULL,
        0
    );

    Serial.println("[SETUP] Sistema inicializado correctamente");
    Serial.println("Comandos MQTT disponibles:");
    Serial.println("- esp32/auditorium/lights/{1-8}/set");
    Serial.println("- esp32/auditorium/lights/all/set");
    Serial.println("- esp32/auditorium/scenario/set");
    Serial.println("=================================");
}

void loop() {
    // Mantener la conexión MQTT y manejar mensajes entrantes
    mqtt_loop();
    
    // Liberar CPU brevemente para otras tareas
    vTaskDelay(pdMS_TO_TICKS(100));
}