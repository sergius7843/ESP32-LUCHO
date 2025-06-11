# ESP32 Memory & WiFi Monitor

Este proyecto está desarrollado para microcontroladores **ESP32** usando **PlatformIO** y el **framework Arduino**, y tiene como objetivo monitorizar y reportar:

- 📶 Información de conexión WiFi
- 📊 Uso de memoria RAM (heap) y memoria Flash (SPIFFS)
- ⏱️ Tiempo desde el encendido del dispositivo (uptime)

Toda esta información se **envía en formato JSON a un broker MQTT**, permitiendo su visualización en dashboards como **Node-RED, MQTT Explorer o Grafana**.

---

## 📦 Características actuales

✅ Modularizado:
- `wifi_manager` → conexión WiFi
- `memory_monitor` → estado de RAM, Flash y uptime
- `json_formatter` → empaquetado en JSON
- `mqtt_client` → conexión y publicación MQTT

✅ Información enviada por MQTT:
- `esp32/wifi/status`: estado de conexión, IP, RSSI, MAC, etc.
- `esp32/system/memory`: RAM utilizada/libre, uso de Flash, uptime

✅ Basado en FreeRTOS:
- Dos tareas independientes manejan los reportes (WiFi y memoria)

✅ Estructura preparada para producción:
- `.gitignore` para evitar subir credenciales y archivos de build
- Código pensado para crecimiento futuro (comandos, nuevos sensores, etc.)

---

## 🧠 Requisitos

- ESP32 Dev Module
- Broker MQTT (como Mosquitto) en red local
- PlatformIO + VS Code

---

## ⚙️ Configuración en `config.h`

```cpp
#define WIFI_SSID       "TuSSID"
#define WIFI_PASSWORD   "TuPassword"

#define MQTT_BROKER     "192.168.0.100"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "esp32_device_01"

#define MQTT_TOPIC_WIFI   "esp32/wifi/status"
#define MQTT_TOPIC_MEMORY "esp32/system/memory"

#define WIFI_INFO_INTERVAL 5000  // En ms

## Salida (monitor serial):

[wifi/status]
{
  "connected": true,
  "ssid": "Aqui aparecera el nombre de tu red wifi",
  "ip": "aqui aparece la IP",
  "mac": "Aqui tu direccion MAC",
  "rssi": -46
}
[system/memory]
{
  "memory": {
    "heap": {
      "total": 330992,
      "free": 241656,
      "used": 89336
    },
    "flash": {
      "total": 1318001,
      "used": 0,
      "free": 1318001
    },
    "uptime_ms": 1303723
  }
}
