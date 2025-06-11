// mqtt_client.cpp - VERSIÓN ACTUALIZADA PARA CONTROL DE LUCES

#include "mqtt_client.h"
#include "config.h"
#include "light_controller.h"  // <- Controlador de luces actualizado
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// ===== CALLBACK MEJORADO PARA LUCES =====
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Mensaje recibido en: ");
    Serial.print(topic);
    Serial.print(" | Payload: ");
    
    // Debug: mostrar payload como string
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    Serial.println(payloadStr);
    
    // Procesar mensajes de control de luces
    String topicStr = String(topic);
    if (topicStr.startsWith("esp32/auditorium/lights/") && topicStr.endsWith("/set")) {
        Serial.println("[MQTT] Procesando comando de luz individual");
        handle_light_message(topic, payload, length);
    }
    else if (topicStr == "esp32/auditorium/lights/all/set") {
        Serial.println("[MQTT] Procesando comando global de luces");
        handle_light_message(topic, payload, length);
    }
    else if (topicStr == "esp32/auditorium/scenario/set") {
        Serial.println("[MQTT] Procesando comando de escenario");
        handle_light_message(topic, payload, length);
    }
    // Mantener compatibilidad con LEDs RGB si los tienes
    else if (topicStr.startsWith("esp32/leds/") && topicStr.endsWith("/set")) {
        Serial.println("[MQTT] Procesando comando de LED RGB (si aplica)");
        // handle_led_message(topic, payload, length); // Descomenta si tienes LEDs RGB
    }
    else {
        Serial.println("[MQTT] Topic no manejado por callback");
    }
}

void mqtt_reconnect() {
    while (!client.connected()) {
        Serial.print("Conectando a MQTT...");
        
        // ===== CONEXIÓN CON WILL MESSAGE =====
        String willTopic = "esp32/status/lastwill";
        String willMessage = "{\"online\": false, \"reason\": \"unexpected_disconnect\"}";
        
        if (client.connect(MQTT_CLIENT_ID, willTopic.c_str(), 0, true, willMessage.c_str())) {
            Serial.println("CONECTADO");
            
            // ===== SUSCRIBIRSE A TODOS LOS TOPICS DE LUCES =====
            Serial.println("[MQTT] Suscribiéndose a topics de control de luces...");
            
            // Suscribirse a control individual de luces (zonas 1-8)
            bool subs[8];
            for (int i = 1; i <= 8; i++) {
                String topic = "esp32/auditorium/lights/" + String(i) + "/set";
                subs[i-1] = client.subscribe(topic.c_str());
                Serial.printf("[MQTT] Suscripción zona %d: %s\n", i, subs[i-1] ? "OK" : "FAIL");
            }
            
            // Suscribirse a comandos globales
            bool subAll = client.subscribe("esp32/auditorium/lights/all/set");
            bool subScenario = client.subscribe("esp32/auditorium/scenario/set");
            
            Serial.printf("[MQTT] Suscripciones globales: ALL=%s, SCENARIO=%s\n", 
                         subAll ? "OK" : "FAIL",
                         subScenario ? "OK" : "FAIL");
            
            // Suscribirse a comandos generales (opcional)
            client.subscribe("esp32/command");
            
            // Publicar mensaje de conexión exitosa
            String connectMsg = "{\"online\": true, \"client_id\": \"" + String(MQTT_CLIENT_ID) + "\", \"timestamp\": " + String(millis()) + ", \"type\": \"auditorium_controller\"}";
            client.publish("esp32/status/connection", connectMsg.c_str(), true);
            
            // Publicar estado inicial de todas las luces después de conectar
            Serial.println("[MQTT] Publicando estado inicial de luces...");
            delay(1000);
            publish_all_lights_status();
            
        } else {
            Serial.print("FALLO, rc=");
            Serial.print(client.state());
            Serial.println(" | Reintentando en 5 segundos...");
            delay(5000);
        }
    }
}

void mqtt_setup() {
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqtt_callback);  // <- IMPORTANTE: Establecer callback
    
    // Aumentar el buffer para mensajes JSON más grandes
    client.setBufferSize(1024);
    
    Serial.println("[MQTT] Cliente configurado para control de luces");
}

void mqtt_loop() {
    if (!client.connected()) {
        mqtt_reconnect();
    }
    client.loop();  // <- IMPORTANTE: Procesar mensajes entrantes
}

void mqtt_publish(const char* topic, const String& payload) {
    if (client.connected()) {
        bool result = client.publish(topic, payload.c_str(), true);  // retain = true
        if (result) {
            Serial.printf("[MQTT] ✓ Publicado en %s\n", topic);
        } else {
            Serial.printf("[MQTT] ✗ Error publicando en %s\n", topic);
        }
    } else {
        Serial.println("[MQTT] No conectado, no se puede publicar");
    }
}

bool mqtt_is_connected() {
    return client.connected();
}

// ===== FUNCIÓN DE DEBUG =====
void mqtt_debug_info() {
    Serial.printf("[MQTT] Estado: %s | Broker: %s:%d | Cliente: %s\n",
                  client.connected() ? "CONECTADO" : "DESCONECTADO",
                  MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
}