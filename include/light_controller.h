// include/light_controller.h
#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>

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
// MQTT Topics para Control
// ============================
#define MQTT_TOPIC_LIGHT_BASE       "esp32/auditorium/lights"
#define MQTT_TOPIC_LIGHT_ALL_SET    "esp32/auditorium/lights/all/set"
#define MQTT_TOPIC_SCENARIO_SET     "esp32/auditorium/scenario/set"
#define MQTT_TOPIC_FAN_SET          "esp32/auditorium/fan/set"
#define MQTT_TOPIC_FAN_STATE        "esp32/auditorium/fan/state"

// Estados de las luces
struct LightState {
    bool isOn;
    unsigned long lastUpdate;
    int pin;
    String name;
};

// Estado del ventilador
struct FanState {
    bool isOn;
    int speed;
    bool autoMode;
    unsigned long lastUpdate;
};

// Funciones principales
void light_controller_setup();
void handle_light_message(char* topic, byte* payload, unsigned int length);

// Control individual de luces
void turn_on_light(int zone);
void turn_off_light(int zone);
void toggle_light(int zone);
bool is_light_on(int zone);

// Control global de luces
void turn_on_all_lights();
void turn_off_all_lights();
void toggle_all_lights();

// Control del ventilador
void turn_on_fan();
void turn_off_fan();
void toggle_fan();
void set_fan_auto_mode(bool enabled);
bool is_fan_on();

// Escenarios predefinidos
void set_scenario_all_on();
void set_scenario_all_off();
void set_scenario_stage_only();
void set_scenario_hallways_only();

// Publicación de estados
void publish_light_status(int zone);
void publish_all_lights_status();
void publish_fan_status();

// Automatización
void check_temperature_automation(float temperature);
void check_ldr_automation(int ldrValue);

// Getters para estado
LightState get_light_state(int zone);
FanState get_fan_state();

#endif // LIGHT_CONTROLLER_H