// src/main.cpp

#include <Arduino.h>
#include "wifi_manager.h"
#include "json_formatter.h"
#include "memory_monitor.h"
#include "mqtt_client.h"
#include "config.h"
#include "status_reporter.h"
#include "temperature_sensor.h"
#include "ldr_sensor.h"
#include "light_controller.h"  // <-- NUEVO: Incluir el controlador de luces

// Variables globales para automatización
float lastTemperature = 0.0;
int lastLdrValue = 0;
unsigned long lastAutomationCheck = 0;

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

// NUEVA TAREA: Automatización basada en sensores
void automationTask(void *parameter) {
    while (true) {
        // Chequear automatización solo si hay datos de sensores
        if (lastTemperature > 0 || lastLdrValue > 0) {
            check_temperature_automation(lastTemperature);
            check_ldr_automation(lastLdrValue);
        }
        
        vTaskDelay(pdMS_TO_TICKS(AUTOMATION_CHECK_INTERVAL));
    }
}

// NUEVA TAREA: Monitoreo de sensores para automatización
void sensorMonitorTask(void *parameter) {
    while (true) {
        // Leer temperatura
        analogReadResolution(ADC_RESOLUTION_BITS);
        int tempRaw = analogRead(TEMP_SENSOR_PIN);
        float voltage = (tempRaw / float(ADC_MAX_VALUE)) * ADC_REF_VOLTAGE;
        lastTemperature = voltage * 100.0;  // LM35: 10mV/°C
        
        // Leer LDR
        int ldrRaw = analogRead(LDR_SENSOR_PIN);
        lastLdrValue = ldrRaw;
        
        // Debug cada 10 segundos
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 10000) {
            Serial.printf("[SENSOR_MONITOR] Temp: %.1f°C, LDR: %d\n", lastTemperature, lastLdrValue);
            lastDebug = millis();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Leer cada segundo
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("========================================");
    Serial.println("🏢 ESP32 AUDITORIUM CONTROLLER v2.0");
    Serial.println("========================================");

    // Inicializar WiFi y MQTT
    Serial.println("[SETUP] Inicializando WiFi...");
    wifi_init();    // Inicializa y conecta a WiFi
    
    Serial.println("[SETUP] Configurando MQTT...");
    mqtt_setup();   // Configura servidor y callback MQTT
    
    // NUEVO: Inicializar control de luces y ventilador
    Serial.println("[SETUP] Inicializando control de luces y ventilador...");
    light_controller_setup();

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

    // Tarea para el sensor de LDR
    start_ldr_task();
    
    // NUEVAS TAREAS: Monitoreo de sensores y automatización
    xTaskCreatePinnedToCore(
        sensorMonitorTask,
        "SensorMonitorTask",
        4096,
        NULL,
        2,  // Prioridad más alta
        NULL,
        0   // Core 0
    );
    
    xTaskCreatePinnedToCore(
        automationTask,
        "AutomationTask",
        4096,
        NULL,
        1,
        NULL,
        0   // Core 0
    );

    Serial.println("[SETUP] ✓ Inicialización completada");
    Serial.println("========================================");
    
    // Esperar a que MQTT se conecte y publicar estado inicial
    delay(3000);
    if (mqtt_is_connected()) {
        publish_all_lights_status();
        publish_fan_status();
        Serial.println("[SETUP] ✓ Estados iniciales publicados");
    }
}

void loop() {
    // Mantener la conexión MQTT y manejar mensajes entrantes
    mqtt_loop();
    
    // Liberar CPU brevemente para otras tareas
    vTaskDelay(pdMS_TO_TICKS(100));
}