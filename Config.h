#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// ESP32-S3-Relay-6CH ThingsBoard Firmware
// Production-Ready Configuration
// ============================================

// --- Firmware Version ---
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_TYPE "ESP32-S3-Relay-6CH"

// --- GPIO Pin Definitions ---
#define GPIO_RELAY_1    1
#define GPIO_RELAY_2    2
#define GPIO_RELAY_3    41
#define GPIO_RELAY_4    42
#define GPIO_RELAY_5    45
#define GPIO_RELAY_6    46

#define GPIO_RGB_LED    38
#define GPIO_BUZZER     21

#define GPIO_RS485_TX   17
#define GPIO_RS485_RX   18

#define GPIO_I2C_SDA    4
#define GPIO_I2C_SCL    5

// --- Relay Configuration ---
#define RELAY_COUNT     6
#define RELAY_ON        HIGH
#define RELAY_OFF       LOW

// --- WiFi AP Mode (Configuration) ---
#define AP_SSID         "ESP32-Relay-Setup"
#define AP_PASSWORD     "12345678"
#define AP_TIMEOUT_MS   180000  // 3 dakika sonra AP kapanır

// --- ThingsBoard Defaults ---
#define TB_PORT_DEFAULT 1883
#define TB_TELEMETRY_TOPIC    "v1/devices/me/telemetry"
#define TB_ATTRIBUTES_TOPIC   "v1/devices/me/attributes"
#define TB_RPC_REQUEST_TOPIC  "v1/devices/me/rpc/request/+"
#define TB_RPC_RESPONSE_TOPIC "v1/devices/me/rpc/response/"

// --- Timing Configuration ---
#define TELEMETRY_INTERVAL_MS   30000   // 30 saniye
#define HEARTBEAT_INTERVAL_MS   60000   // 1 dakika
#define MQTT_RECONNECT_DELAY_MS 5000    // 5 saniye
#define WIFI_RECONNECT_DELAY_MS 10000   // 10 saniye
#define WATCHDOG_TIMEOUT_S      30      // 30 saniye

// --- NVS Keys ---
#define NVS_NAMESPACE       "relay_config"
#define NVS_KEY_WIFI_SSID   "wifi_ssid"
#define NVS_KEY_WIFI_PASS   "wifi_pass"
#define NVS_KEY_TB_SERVER   "tb_server"
#define NVS_KEY_TB_PORT     "tb_port"
#define NVS_KEY_TB_TOKEN    "tb_token"
#define NVS_KEY_CONFIGURED  "configured"

// --- LED Status Colors (RGB) ---
#define LED_COLOR_OFF       0x000000
#define LED_COLOR_RED       0xFF0000  // Hata
#define LED_COLOR_GREEN     0x00FF00  // Bağlı
#define LED_COLOR_BLUE      0x0000FF  // AP Mode
#define LED_COLOR_YELLOW    0xFFFF00  // WiFi bağlanıyor
#define LED_COLOR_CYAN      0x00FFFF  // MQTT bağlanıyor
#define LED_COLOR_PURPLE    0xFF00FF  // OTA update
#define LED_COLOR_WHITE     0xFFFFFF  // Boot

// --- Debug ---
#define DEBUG_ENABLED       true
#define SERIAL_BAUD         115200

#if DEBUG_ENABLED
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
