#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>

// Estructura para definir cada zona de iluminación
struct LightZone {
    int id;
    int gpio_pin;
    const char* name;
    const char* description;
    bool isOn;
    unsigned long lastUpdate;
};

// Funciones principales
void init_light_controller();
void handle_light_message(char* topic, byte* payload, unsigned int length);
void toggle_light(int zone_id);
void turn_on_light(int zone_id);
void turn_off_light(int zone_id);
void turn_on_all_lights();
void turn_off_all_lights();
void set_scenario(const char* scenario);
void publish_light_status(int zone_id);
void publish_all_lights_status();

// Funciones de utilidad
LightZone* get_zone_by_id(int zone_id);
bool is_valid_zone(int zone_id);
int get_total_zones();
int get_active_zones();

#endif