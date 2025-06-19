#ifndef CONFIG_H
#define CONFIG_H

// ============================
//WiFi Configuration
// ============================
#define WIFI_SSID        "iPhone"
#define WIFI_PASSWORD    "88888888"

// Tiempo de actualización de datos en ms (WiFi, Memoria)
#define WIFI_INFO_INTERVAL         8000  // ms

// ============================
//MQTT Configuration
// ============================
#define MQTT_BROKER       "172.20.10.2"   // Dirección del broker MQTT
#define MQTT_PORT         1883             // Puerto para MQTT (no WebSockets)
#define MQTT_CLIENT_ID    "esp32_device_01"

// Intervalo de latido (heartbeat) en ms
#define MQTT_HEARTBEAT_INTERVAL   2500    // ms

// ============================
//MQTT Topics
// ============================

// Información del estado de conexión WiFi
#define MQTT_TOPIC_WIFI       "esp32/wifi/status"

// Información de uso de memoria (RAM, Flash, Uptime)
#define MQTT_TOPIC_MEMORY     "esp32/system/memory"

// Estado de conexión al broker (heartbeat)
#define MQTT_TOPIC_HEARTBEAT  "esp32/status/heartbeat"

//MQTT temperatura
#define MQTT_TOPIC_TEMPERATURE "esp32/sensors/temperature"

// ============================
// 🌡️ Sensor de Temperatura (ADC)
// ============================
#define TEMP_SENSOR_PIN           34
#define TEMP_REPORT_INTERVAL      5000      // ms
#define ADC_RESOLUTION_BITS       12
#define ADC_REF_VOLTAGE           3.3
#define ADC_MAX_VALUE             4095      // para 12 bits

// ============================
// 🌙 Sensor LDR (luminosidad)
// ============================
#define LDR_SENSOR_PIN         35      // Pin ADC donde conectas el LDR
#define LDR_REPORT_INTERVAL    5000    // ms (igual que temperatura)
#define MQTT_TOPIC_LDR         "esp32/sensors/ldr"

// ============================
// 💡 Control de Luces (Relés)
// ============================
#define LIGHT_ZONE_1_PIN    23  // Escenario Principal
#define LIGHT_ZONE_2_PIN    22  // Pasillo Derecho - A
#define LIGHT_ZONE_3_PIN    21  // Pasillo Derecho - B
#define LIGHT_ZONE_4_PIN    19  // Pasillo Izquierdo

// ============================
// 🌀 Control del Ventilador
// ============================
#define FAN_CONTROL_PIN     18  // L298N control pin

// ============================
// Intervalos de Automatización
// ============================
#define AUTOMATION_CHECK_INTERVAL  2000  // ms - Chequear automatización cada 2 segundos

#endif // CONFIG_H