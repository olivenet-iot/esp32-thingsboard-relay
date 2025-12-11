/*
 * ============================================
 * ESP32-S3-Relay-6CH ThingsBoard Firmware
 * ============================================
 * 
 * Production-Ready Industrial Relay Controller
 * 
 * Features:
 * - WiFi configuration via captive portal (AP mode)
 * - ThingsBoard MQTT integration with RPC support
 * - 6-channel relay control
 * - OTA firmware updates
 * - RGB LED status indicator
 * - Buzzer feedback
 * - Watchdog timer
 * - Auto-reconnect
 * 
 * Author: Olivenet Ltd.
 * Version: 1.0.0
 * 
 * Hardware: Waveshare ESP32-S3-Relay-6CH
 * 
 * ============================================
 */

#include <WiFi.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Config.h"
#include "RelayController.h"
#include "StatusLED.h"
#include "ConfigManager.h"
#include "ThingsBoardMQTT.h"
#include "OTAHandler.h"
#include "Buzzer.h"

// ============================================
// State Machine
// ============================================
enum class DeviceState {
    BOOT,
    AP_MODE,
    WIFI_CONNECTING,
    MQTT_CONNECTING,
    CONNECTED,
    ERROR
};

DeviceState currentState = DeviceState::BOOT;
unsigned long stateEnteredAt = 0;
unsigned long lastWiFiAttempt = 0;
int wifiRetryCount = 0;

#define WIFI_MAX_RETRIES 10
#define WIFI_RETRY_INTERVAL_MS 5000

// ============================================
// Forward declarations
// ============================================
void changeState(DeviceState newState);
void handleBoot();
void handleAPMode();
void handleWiFiConnecting();
void handleMQTTConnecting();
void handleConnected();
void handleError();
void onRelayChange(uint8_t channel, bool state);

// ============================================
// Setup
// ============================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    
    DEBUG_PRINTLN("\n\n");
    DEBUG_PRINTLN("============================================");
    DEBUG_PRINTLN("  ESP32-S3-Relay-6CH ThingsBoard Firmware");
    DEBUG_PRINTF("  Version: %s\n", FIRMWARE_VERSION);
    DEBUG_PRINTLN("============================================\n");
    
    // Watchdog başlat (ESP-IDF 5.x API)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WATCHDOG_TIMEOUT_S * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL);
    
    // Modülleri başlat
    Led.begin();
    Led.setStatus(LedStatus::BOOT);
    
    Buzz.begin();
    Buzz.bootSound();
    
    Relays.begin();
    Relays.setOnChangeCallback(onRelayChange);
    
    Config.begin();
    
    // Boot durumuna geç
    changeState(DeviceState::BOOT);
}

// ============================================
// Main Loop
// ============================================
void loop() {
    // Watchdog besle
    esp_task_wdt_reset();
    
    // LED güncelle
    Led.update();
    
    // Durum makinesi
    switch (currentState) {
        case DeviceState::BOOT:
            handleBoot();
            break;
            
        case DeviceState::AP_MODE:
            handleAPMode();
            break;
            
        case DeviceState::WIFI_CONNECTING:
            handleWiFiConnecting();
            break;
            
        case DeviceState::MQTT_CONNECTING:
            handleMQTTConnecting();
            break;
            
        case DeviceState::CONNECTED:
            handleConnected();
            break;
            
        case DeviceState::ERROR:
            handleError();
            break;
    }
}

// ============================================
// State Handlers
// ============================================

void changeState(DeviceState newState) {
    if (currentState == newState) return;
    
    DEBUG_PRINTF("[State] %d -> %d\n", (int)currentState, (int)newState);
    
    currentState = newState;
    stateEnteredAt = millis();
    
    switch (newState) {
        case DeviceState::BOOT:
            Led.setStatus(LedStatus::BOOT);
            break;
            
        case DeviceState::AP_MODE:
            Led.setStatus(LedStatus::AP_MODE);
            Config.startAPMode();
            DEBUG_PRINTLN("\n>>> AP Mode: Connect to WiFi 'ESP32-Relay-Setup' with password '12345678' <<<\n");
            break;
            
        case DeviceState::WIFI_CONNECTING:
            Led.setStatus(LedStatus::WIFI_CONNECTING);
            wifiRetryCount = 0;
            lastWiFiAttempt = 0;
            break;
            
        case DeviceState::MQTT_CONNECTING:
            Led.setStatus(LedStatus::MQTT_CONNECTING);
            TB.begin();
            break;
            
        case DeviceState::CONNECTED:
            Led.setStatus(LedStatus::CONNECTED);
            OTA.begin();
            Buzz.successSound();
            DEBUG_PRINTLN("\n>>> CONNECTED - System Ready <<<\n");
            break;
            
        case DeviceState::ERROR:
            Led.setStatus(LedStatus::ERROR);
            Buzz.errorSound();
            break;
    }
}

void handleBoot() {
    delay(500);
    
    if (Config.isConfigured()) {
        DEBUG_PRINTLN("[Boot] Configuration found, connecting to WiFi...");
        changeState(DeviceState::WIFI_CONNECTING);
    } else {
        DEBUG_PRINTLN("[Boot] No configuration, starting AP mode...");
        changeState(DeviceState::AP_MODE);
    }
}

void handleAPMode() {
    Config.handlePortal();
    
    // AP mode timeout
    if (!Config.isAPModeActive()) {
        if (Config.isConfigured()) {
            changeState(DeviceState::WIFI_CONNECTING);
        } else {
            // Timeout oldu ama hala config yok - tekrar AP mode
            changeState(DeviceState::AP_MODE);
        }
    }
}

void handleWiFiConnecting() {
    // WiFi bağlı mı kontrol et
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTF("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        changeState(DeviceState::MQTT_CONNECTING);
        return;
    }
    
    unsigned long now = millis();
    
    // Bağlantı denemesi
    if (now - lastWiFiAttempt > WIFI_RETRY_INTERVAL_MS || lastWiFiAttempt == 0) {
        lastWiFiAttempt = now;
        wifiRetryCount++;
        
        if (wifiRetryCount > WIFI_MAX_RETRIES) {
            DEBUG_PRINTLN("[WiFi] Max retries reached, going to AP mode");
            WiFi.disconnect();
            changeState(DeviceState::AP_MODE);
            return;
        }
        
        DeviceConfig& cfg = Config.getConfig();
        DEBUG_PRINTF("[WiFi] Connecting to '%s' (attempt %d/%d)...\n", 
                     cfg.wifiSsid, wifiRetryCount, WIFI_MAX_RETRIES);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg.wifiSsid, cfg.wifiPassword);
    }
}

void handleMQTTConnecting() {
    // WiFi koptu mu?
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("[MQTT] WiFi lost, reconnecting...");
        changeState(DeviceState::WIFI_CONNECTING);
        return;
    }
    
    // MQTT bağlantı dene
    if (TB.connect()) {
        changeState(DeviceState::CONNECTED);
    } else {
        // 5 saniye bekle ve tekrar dene
        delay(MQTT_RECONNECT_DELAY_MS);
    }
}

void handleConnected() {
    // WiFi koptu mu?
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("[Connected] WiFi lost!");
        TB.disconnect();
        changeState(DeviceState::WIFI_CONNECTING);
        return;
    }
    
    // MQTT koptu mu?
    if (!TB.isConnected()) {
        DEBUG_PRINTLN("[Connected] MQTT disconnected!");
        changeState(DeviceState::MQTT_CONNECTING);
        return;
    }
    
    // Normal işlemler
    TB.loop();
    OTA.loop();
}

void handleError() {
    // 10 saniye bekle ve yeniden başla
    if (millis() - stateEnteredAt > 10000) {
        ESP.restart();
    }
}

// ============================================
// Callbacks
// ============================================

void onRelayChange(uint8_t channel, bool state) {
    DEBUG_PRINTF("[Callback] Relay %d changed to %s\n", channel, state ? "ON" : "OFF");
    
    // LED flash
    Led.flash(state ? LED_COLOR_GREEN : LED_COLOR_RED, 100);
    
    // Buzzer click
    Buzz.clickSound();
    
    // MQTT bağlıysa telemetry gönder
    if (currentState == DeviceState::CONNECTED && TB.isConnected()) {
        TB.sendTelemetry();
    }
}
