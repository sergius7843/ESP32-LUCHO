// src/light_controller.cpp
#include "light_controller.h"
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

// Estados globales
LightState lightStates[4] = {
    {false, 0, LIGHT_ZONE_1_PIN, "Escenario Principal"},
    {false, 0, LIGHT_ZONE_2_PIN, "Pasillo Derecho - A"},
    {false, 0, LIGHT_ZONE_3_PIN, "Pasillo Derecho - B"},
    {false, 0, LIGHT_ZONE_4_PIN, "Pasillo Izquierdo"}
};

FanState fanState = {false, 0, true, 0};

// Configuración de automatización
struct AutomationConfig {
    bool ldrAutoMode = false;
    bool temperatureAutoMode = true;
    float tempHotThreshold = 20.0;
    float tempColdThreshold = 15.0;
    int ldrDarkThreshold = 3000;
    int ldrBrightThreshold = 1000;
} automationConfig;

// ============================
// INICIALIZACIÓN
// ============================
void light_controller_setup() {
    Serial.println("[LIGHT_CONTROLLER] Inicializando control de luces y ventilador...");
    
    // Configurar pines de relés (luces)
    for (int i = 0; i < 4; i++) {
        pinMode(lightStates[i].pin, OUTPUT);
        digitalWrite(lightStates[i].pin, LOW); // Relés activos en LOW
        lightStates[i].isOn = false;
        lightStates[i].lastUpdate = millis();
        Serial.printf("[LIGHT] Zona %d configurada en pin %d\n", i+1, lightStates[i].pin);
    }
    
    // Configurar pin del ventilador
    pinMode(FAN_CONTROL_PIN, OUTPUT);
    digitalWrite(FAN_CONTROL_PIN, LOW);
    fanState.isOn = false;
    fanState.lastUpdate = millis();
    
    Serial.println("[LIGHT_CONTROLLER] ✓ Inicialización completada");
    
    // Publicar estado inicial después de un delay
    delay(2000);
    publish_all_lights_status();
    publish_fan_status();
}

// ============================
// MANEJO DE MENSAJES MQTT
// ============================
void handle_light_message(char* topic, byte* payload, unsigned int length) {
    // Convertir payload a string
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    String topicStr = String(topic);
    Serial.printf("[LIGHT_HANDLER] Topic: %s | Payload: %s\n", topic, payloadStr.c_str());
    
    // Parsear JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payloadStr);
    
    if (error) {
        Serial.printf("[LIGHT_HANDLER] ✗ Error parsing JSON: %s\n", error.c_str());
        return;
    }
    
    String command = doc["command"] | "";
    command.toUpperCase();
    
    // ===== CONTROL INDIVIDUAL DE LUCES =====
    if (topicStr.startsWith("esp32/auditorium/lights/") && topicStr.endsWith("/set")) {
        // Extraer número de zona del topic
        int zoneStart = topicStr.indexOf("/lights/") + 8;
        int zoneEnd = topicStr.indexOf("/set");
        String zoneStr = topicStr.substring(zoneStart, zoneEnd);
        int zone = zoneStr.toInt();
        
        if (zone >= 1 && zone <= 4) {
            if (command == "ON") {
                turn_on_light(zone);
            } else if (command == "OFF") {
                turn_off_light(zone);
            } else if (command == "TOGGLE") {
                toggle_light(zone);
            }
            publish_light_status(zone);
        } else {
            Serial.printf("[LIGHT_HANDLER] ✗ Zona inválida: %d\n", zone);
        }
    }
    
    // ===== CONTROL GLOBAL DE LUCES =====
    else if (topicStr == "esp32/auditorium/lights/all/set") {
        if (command == "ON") {
            turn_on_all_lights();
        } else if (command == "OFF") {
            turn_off_all_lights();
        } else if (command == "TOGGLE") {
            toggle_all_lights();
        }
        publish_all_lights_status();
    }
    
    // ===== ESCENARIOS =====
    else if (topicStr == "esp32/auditorium/scenario/set") {
        String scenario = doc["scenario"] | "";
        scenario.toLowerCase();
        
        if (scenario == "all_on" || scenario == "todo_encendido") {
            set_scenario_all_on();
        } else if (scenario == "all_off" || scenario == "todo_apagado") {
            set_scenario_all_off();
        } else if (scenario == "stage_only" || scenario == "solo_escenario") {
            set_scenario_stage_only();
        } else if (scenario == "hallways_only" || scenario == "solo_pasillos") {
            set_scenario_hallways_only();
        }
        publish_all_lights_status();
    }
    
    // ===== CONTROL DEL VENTILADOR =====
    else if (topicStr == "esp32/auditorium/fan/set") {
        if (command == "ON") {
            turn_on_fan();
        } else if (command == "OFF") {
            turn_off_fan();
        } else if (command == "TOGGLE") {
            toggle_fan();
        }
        
        // Configurar modo automático si se especifica
        if (doc.containsKey("auto_mode")) {
            bool autoMode = doc["auto_mode"];
            set_fan_auto_mode(autoMode);
        }
        
        publish_fan_status();
    }
}

// ============================
// CONTROL INDIVIDUAL DE LUCES
// ============================
void turn_on_light(int zone) {
    if (zone < 1 || zone > 4) return;
    
    int index = zone - 1;
    digitalWrite(lightStates[index].pin, HIGH); // Activar relé
    lightStates[index].isOn = true;
    lightStates[index].lastUpdate = millis();
    
    Serial.printf("[LIGHT] ✓ Zona %d (%s) ENCENDIDA\n", zone, lightStates[index].name.c_str());
}

void turn_off_light(int zone) {
    if (zone < 1 || zone > 4) return;
    
    int index = zone - 1;
    digitalWrite(lightStates[index].pin, LOW); // Desactivar relé
    lightStates[index].isOn = false;
    lightStates[index].lastUpdate = millis();
    
    Serial.printf("[LIGHT] ✓ Zona %d (%s) APAGADA\n", zone, lightStates[index].name.c_str());
}

void toggle_light(int zone) {
    if (zone < 1 || zone > 4) return;
    
    int index = zone - 1;
    if (lightStates[index].isOn) {
        turn_off_light(zone);
    } else {
        turn_on_light(zone);
    }
}

bool is_light_on(int zone) {
    if (zone < 1 || zone > 4) return false;
    return lightStates[zone - 1].isOn;
}

// ============================
// CONTROL GLOBAL DE LUCES
// ============================
void turn_on_all_lights() {
    Serial.println("[LIGHT] Encendiendo todas las luces...");
    for (int i = 1; i <= 4; i++) {
        turn_on_light(i);
        delay(100); // Pequeño delay entre activaciones
    }
}

void turn_off_all_lights() {
    Serial.println("[LIGHT] Apagando todas las luces...");
    for (int i = 1; i <= 4; i++) {
        turn_off_light(i);
        delay(100);
    }
}

void toggle_all_lights() {
    // Si alguna está encendida, apagar todas; si todas están apagadas, encender todas
    bool anyOn = false;
    for (int i = 0; i < 4; i++) {
        if (lightStates[i].isOn) {
            anyOn = true;
            break;
        }
    }
    
    if (anyOn) {
        turn_off_all_lights();
    } else {
        turn_on_all_lights();
    }
}

// ============================
// CONTROL DEL VENTILADOR
// ============================
void turn_on_fan() {
    digitalWrite(FAN_CONTROL_PIN, HIGH);  // ✓ Corregido: usar FAN_CONTROL_PIN consistentemente
    fanState.isOn = true;
    fanState.lastUpdate = millis();
    Serial.println("[FAN] ✓ Ventilador ENCENDIDO");
}

void turn_off_fan() {
    digitalWrite(FAN_CONTROL_PIN, LOW);
    fanState.isOn = false;
    fanState.speed = 0;
    fanState.lastUpdate = millis();
    Serial.println("[FAN] ✓ Ventilador APAGADO");
}

void toggle_fan() {
    if (fanState.isOn) {
        turn_off_fan();
    } else {
        turn_on_fan();
    }
}

void set_fan_auto_mode(bool enabled) {
    fanState.autoMode = enabled;
    Serial.printf("[FAN] Modo automático: %s\n", enabled ? "ACTIVADO" : "DESACTIVADO");
}

bool is_fan_on() {
    return fanState.isOn;
}

// ============================
// ESCENARIOS PREDEFINIDOS
// ============================
void set_scenario_all_on() {
    Serial.println("[SCENARIO] Ejecutando: Todas las luces encendidas");
    turn_on_all_lights();
}

void set_scenario_all_off() {
    Serial.println("[SCENARIO] Ejecutando: Todas las luces apagadas");
    turn_off_all_lights();
}

void set_scenario_stage_only() {
    Serial.println("[SCENARIO] Ejecutando: Solo escenario");
    turn_off_all_lights();
    delay(500);
    turn_on_light(1); // Solo escenario principal
}

void set_scenario_hallways_only() {
    Serial.println("[SCENARIO] Ejecutando: Solo pasillos");
    turn_off_light(1); // Apagar escenario
    delay(200);
    turn_on_light(2);  // Pasillo derecho A
    turn_on_light(3);  // Pasillo derecho B
    turn_on_light(4);  // Pasillo izquierdo
}

// ============================
// PUBLICACIÓN DE ESTADOS
// ============================
void publish_light_status(int zone) {
    if (zone < 1 || zone > 4 || !mqtt_is_connected()) return;
    
    int index = zone - 1;
    StaticJsonDocument<200> doc;
    
    doc["zone"] = zone;
    doc["name"] = lightStates[index].name;
    doc["status"] = lightStates[index].isOn ? "ON" : "OFF";
    doc["timestamp"] = lightStates[index].lastUpdate;
    doc["pin"] = lightStates[index].pin;
    
    String json;
    serializeJson(doc, json);
    
    String topic = "esp32/auditorium/lights/" + String(zone) + "/state";
    mqtt_publish(topic.c_str(), json);
}

void publish_all_lights_status() {
    for (int i = 1; i <= 4; i++) {
        publish_light_status(i);
        delay(100); // Evitar saturar MQTT
    }
    
    // Publicar estado global
    if (mqtt_is_connected()) {
        StaticJsonDocument<300> doc;
        doc["total_zones"] = 4;
        doc["active_zones"] = 0;
        
        JsonArray zones = doc.createNestedArray("zones");
        for (int i = 0; i < 4; i++) {
            JsonObject zone = zones.createNestedObject();
            zone["id"] = i + 1;
            zone["name"] = lightStates[i].name;
            zone["status"] = lightStates[i].isOn ? "ON" : "OFF";
            if (lightStates[i].isOn) {
                doc["active_zones"] = doc["active_zones"].as<int>() + 1;
            }
        }
        
        bool allOn = doc["active_zones"].as<int>() == 4;
        doc["all_lights_on"] = allOn;
        doc["timestamp"] = millis();
        
        String json;
        serializeJson(doc, json);
        mqtt_publish("esp32/auditorium/lights/status", json);
    }
}

void publish_fan_status() {
    if (!mqtt_is_connected()) return;
    
    StaticJsonDocument<200> doc;
    doc["status"] = fanState.isOn ? "ON" : "OFF";
    doc["speed"] = fanState.speed;
    doc["auto_mode"] = fanState.autoMode;
    doc["timestamp"] = fanState.lastUpdate;
    doc["pin"] = FAN_CONTROL_PIN;
    
    String json;
    serializeJson(doc, json);
    mqtt_publish(MQTT_TOPIC_FAN_STATE, json);
}

// ============================
// AUTOMATIZACIÓN
// ============================
void check_temperature_automation(float temperature) {
    // ✓ Corregido: Solo actuar si el modo automático está habilitado
    if (!automationConfig.temperatureAutoMode || !fanState.autoMode) return;
    
    // ✓ Agregado: Evitar conflictos con control manual
    static unsigned long lastAutoAction = 0;
    unsigned long currentTime = millis();
    
    // Si el ventilador fue controlado manualmente recientemente (últimos 30 segundos), no intervenir
    if (currentTime - fanState.lastUpdate < 30000) {
        return;
    }
    
    if (temperature >= automationConfig.tempHotThreshold && !fanState.isOn) {
        Serial.printf("[AUTO] Temperatura alta detectada (%.1f°C) - Encendiendo ventilador\n", temperature);
        turn_on_fan();
        lastAutoAction = currentTime;
        publish_fan_status();
    } else if (temperature <= automationConfig.tempColdThreshold && fanState.isOn) {
        // Solo apagar automáticamente si la última acción fue automática
        if (currentTime - lastAutoAction < 300000) { // 5 minutos
            Serial.printf("[AUTO] Temperatura normal (%.1f°C) - Apagando ventilador\n", temperature);
            turn_off_fan();
            publish_fan_status();
        }
    }
}

void check_ldr_automation(int ldrValue) {
    if (!automationConfig.ldrAutoMode) return;
    
    // Contar luces encendidas
    int lightsOn = 0;
    for (int i = 0; i < 4; i++) {
        if (lightStates[i].isOn) lightsOn++;
    }
    
    if (ldrValue >= automationConfig.ldrDarkThreshold && lightsOn == 0) {
        Serial.printf("[AUTO] Oscuridad detectada (LDR: %d) - Encendiendo luces\n", ldrValue);
        turn_on_all_lights();
        publish_all_lights_status();
    } else if (ldrValue <= automationConfig.ldrBrightThreshold && lightsOn > 0) {
        Serial.printf("[AUTO] Luminosidad alta detectada (LDR: %d) - Apagando luces\n", ldrValue);
        turn_off_all_lights();
        publish_all_lights_status();
    }
}

// ============================
// GETTERS
// ============================
LightState get_light_state(int zone) {
    if (zone < 1 || zone > 4) {
        return {false, 0, -1, "Invalid"};
    }
    return lightStates[zone - 1];
}

FanState get_fan_state() {
    return fanState;
}