#include "light_controller.h"
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

// Definición de las zonas de iluminación del auditorio
LightZone lightZones[] = {
    {1, 23, "Escenario Principal", "Iluminación principal del escenario", false, 0},
    {2, 22, "Público - Sector A", "Iluminación del público lado izquierdo", false, 0},
    {3, 21, "Público - Sector B", "Iluminación del público lado derecho", false, 0},
    {4, 19, "Pasillo Central", "Iluminación del pasillo principal", false, 0},
    {5, 18, "Foyer/Entrada", "Iluminación del área de entrada", false, 0},
    {6, 17, "Emergencia", "Sistema de iluminación de emergencia", false, 0},
    {7, 16, "Zona Técnica", "Iluminación área técnica/cabina", false, 0},
    {8, 4, "Reserva", "Zona de reserva para expansión", false, 0}
};

const int TOTAL_ZONES = sizeof(lightZones) / sizeof(lightZones[0]);

// ===== INICIALIZACIÓN =====
void init_light_controller() {
    Serial.println("[LIGHTS] Inicializando controlador de luces...");
    
    // Configurar todos los pines como salida y apagarlos
    for (int i = 0; i < TOTAL_ZONES; i++) {
        pinMode(lightZones[i].gpio_pin, OUTPUT);
        digitalWrite(lightZones[i].gpio_pin, LOW); // Relé activo bajo = apagado
        lightZones[i].isOn = false;
        lightZones[i].lastUpdate = millis();
        
        Serial.printf("[LIGHTS] Zona %d (%s) configurada en GPIO %d\n", 
                     lightZones[i].id, 
                     lightZones[i].name, 
                     lightZones[i].gpio_pin);
    }
    
    // Publicar estado inicial de todas las luces
    delay(2000); // Esperar que MQTT esté listo
    publish_all_lights_status();
    
    Serial.println("[LIGHTS] Controlador inicializado correctamente");
}

// ===== MANEJO DE MENSAJES MQTT =====
void handle_light_message(char* topic, byte* payload, unsigned int length) {
    Serial.printf("[LIGHTS] Procesando mensaje: %s\n", topic);
    
    // Convertir payload a string
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    Serial.printf("[LIGHTS] Payload: %s\n", payloadStr.c_str());
    
    // Extraer zone_id del topic: esp32/auditorium/lights/{id}/set
    String topicStr = String(topic);
    int zoneId = -1;
    
    // Buscar patrón esp32/auditorium/lights/{id}/set
    int startIdx = topicStr.indexOf("esp32/auditorium/lights/");
    if (startIdx != -1) {
        startIdx += 24; // Longitud de "esp32/auditorium/lights/"
        int endIdx = topicStr.indexOf("/set", startIdx);
        if (endIdx != -1) {
            String zoneIdStr = topicStr.substring(startIdx, endIdx);
            zoneId = zoneIdStr.toInt();
        }
    }
    
    if (zoneId == -1 || !is_valid_zone(zoneId)) {
        Serial.printf("[LIGHTS] ID de zona inválido: %d\n", zoneId);
        return;
    }
    
    // Parsear comando JSON
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payloadStr);
    
    if (error) {
        Serial.printf("[LIGHTS] Error parseando JSON: %s\n", error.c_str());
        return;
    }
    
    // Procesar comando
    if (doc.containsKey("command")) {
        String command = doc["command"].as<String>();
        command.toUpperCase();
        
        Serial.printf("[LIGHTS] Ejecutando comando '%s' en zona %d\n", command.c_str(), zoneId);
        
        if (command == "ON") {
            turn_on_light(zoneId);
        } else if (command == "OFF") {
            turn_off_light(zoneId);
        } else if (command == "TOGGLE") {
            toggle_light(zoneId);
        } else {
            Serial.printf("[LIGHTS] Comando desconocido: %s\n", command.c_str());
        }
    }
    
    // Procesar escenarios globales (si el topic es para todos)
    if (doc.containsKey("scenario")) {
        String scenario = doc["scenario"].as<String>();
        set_scenario(scenario.c_str());
    }
}

// ===== CONTROL INDIVIDUAL DE LUCES =====
void toggle_light(int zone_id) {
    LightZone* zone = get_zone_by_id(zone_id);
    if (zone == nullptr) return;
    
    zone->isOn ? turn_off_light(zone_id) : turn_on_light(zone_id);
}

void turn_on_light(int zone_id) {
    LightZone* zone = get_zone_by_id(zone_id);
    if (zone == nullptr) return;
    
    digitalWrite(zone->gpio_pin, HIGH); // Relé activo alto = encendido
    zone->isOn = true;
    zone->lastUpdate = millis();
    
    Serial.printf("[LIGHTS] Zona %d (%s) ENCENDIDA\n", zone->id, zone->name);
    publish_light_status(zone_id);
}

void turn_off_light(int zone_id) {
    LightZone* zone = get_zone_by_id(zone_id);
    if (zone == nullptr) return;
    
    digitalWrite(zone->gpio_pin, LOW); // Relé activo bajo = apagado
    zone->isOn = false;
    zone->lastUpdate = millis();
    
    Serial.printf("[LIGHTS] Zona %d (%s) APAGADA\n", zone->id, zone->name);
    publish_light_status(zone_id);
}

// ===== CONTROL GLOBAL =====
void turn_on_all_lights() {
    Serial.println("[LIGHTS] Encendiendo todas las luces...");
    for (int i = 0; i < TOTAL_ZONES; i++) {
        turn_on_light(lightZones[i].id);
        delay(100); // Pequeña pausa entre activaciones
    }
}

void turn_off_all_lights() {
    Serial.println("[LIGHTS] Apagando todas las luces...");
    for (int i = 0; i < TOTAL_ZONES; i++) {
        turn_off_light(lightZones[i].id);
        delay(100); // Pequeña pausa entre desactivaciones
    }
}

// ===== ESCENARIOS PREDEFINIDOS =====
void set_scenario(const char* scenario) {
    Serial.printf("[LIGHTS] Aplicando escenario: %s\n", scenario);
    
    String scenarioStr = String(scenario);
    scenarioStr.toLowerCase();
    
    if (scenarioStr == "presentation") {
        // Solo escenario, pasillo y foyer
        turn_on_light(1);  // Escenario
        turn_off_light(2); // Público A
        turn_off_light(3); // Público B
        turn_on_light(4);  // Pasillo
        turn_on_light(5);  // Foyer
        turn_off_light(6); // Emergencia
        turn_off_light(7); // Técnica
        turn_off_light(8); // Reserva
    }
    else if (scenarioStr == "full") {
        // Todas las luces encendidas
        turn_on_all_lights();
    }
    else if (scenarioStr == "break") {
        // Descanso - áreas comunes
        turn_off_light(1); // Escenario
        turn_on_light(2);  // Público A
        turn_on_light(3);  // Público B
        turn_on_light(4);  // Pasillo
        turn_on_light(5);  // Foyer
        turn_off_light(6); // Emergencia
        turn_off_light(7); // Técnica
        turn_off_light(8); // Reserva
    }
    else if (scenarioStr == "emergency") {
        // Solo emergencia, pasillo y foyer
        turn_off_light(1); // Escenario
        turn_off_light(2); // Público A
        turn_off_light(3); // Público B
        turn_on_light(4);  // Pasillo
        turn_on_light(5);  // Foyer
        turn_on_light(6);  // Emergencia
        turn_off_light(7); // Técnica
        turn_off_light(8); // Reserva
    }
    else if (scenarioStr == "off") {
        // Apagar todo
        turn_off_all_lights();
    }
    else {
        Serial.printf("[LIGHTS] Escenario desconocido: %s\n", scenario);
    }
}

// ===== PUBLICACIÓN DE ESTADOS =====
void publish_light_status(int zone_id) {
    LightZone* zone = get_zone_by_id(zone_id);
    if (zone == nullptr) return;
    
    // Crear JSON con el estado de la luz
    DynamicJsonDocument doc(256);
    doc["zone_id"] = zone->id;
    doc["name"] = zone->name;
    doc["description"] = zone->description;
    doc["status"] = zone->isOn ? "ON" : "OFF";
    doc["gpio_pin"] = zone->gpio_pin;
    doc["timestamp"] = millis();
    doc["last_update"] = zone->lastUpdate;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Topic: esp32/auditorium/lights/{id}/state
    String topic = "esp32/auditorium/lights/" + String(zone_id) + "/state";
    mqtt_publish(topic.c_str(), jsonString);
}

void publish_all_lights_status() {
    Serial.println("[LIGHTS] Publicando estado de todas las luces...");
    
    for (int i = 0; i < TOTAL_ZONES; i++) {
        publish_light_status(lightZones[i].id);
        delay(50); // Pequeña pausa entre publicaciones
    }
    
    // Publicar resumen global
    DynamicJsonDocument doc(512);
    doc["total_zones"] = TOTAL_ZONES;
    doc["active_zones"] = get_active_zones();
    doc["all_lights_on"] = (get_active_zones() == TOTAL_ZONES);
    doc["timestamp"] = millis();
    
    JsonArray zones = doc.createNestedArray("zones");
    for (int i = 0; i < TOTAL_ZONES; i++) {
        JsonObject zone = zones.createNestedObject();
        zone["id"] = lightZones[i].id;
        zone["name"] = lightZones[i].name;
        zone["status"] = lightZones[i].isOn ? "ON" : "OFF";
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    mqtt_publish("esp32/auditorium/lights/summary", jsonString);
}

// ===== FUNCIONES DE UTILIDAD =====
LightZone* get_zone_by_id(int zone_id) {
    for (int i = 0; i < TOTAL_ZONES; i++) {
        if (lightZones[i].id == zone_id) {
            return &lightZones[i];
        }
    }
    return nullptr;
}

bool is_valid_zone(int zone_id) {
    return get_zone_by_id(zone_id) != nullptr;
}

int get_total_zones() {
    return TOTAL_ZONES;
}

int get_active_zones() {
    int active = 0;
    for (int i = 0; i < TOTAL_ZONES; i++) {
        if (lightZones[i].isOn) active++;
    }
    return active;
}