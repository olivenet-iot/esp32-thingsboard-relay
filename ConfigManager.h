#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Config.h"

struct DeviceConfig {
    char wifiSsid[64];
    char wifiPassword[64];
    char tbServer[128];
    uint16_t tbPort;
    char tbToken[64];
    bool configured;
};

class ConfigManager {
public:
    ConfigManager();
    
    void begin();
    
    // Konfigürasyon okuma/yazma
    bool loadConfig();
    bool saveConfig();
    void resetConfig();
    
    // AP Mode portal
    void startAPMode();
    void stopAPMode();
    void handlePortal();  // loop() içinde çağrılmalı
    bool isAPModeActive();
    
    // Getter'lar
    DeviceConfig& getConfig();
    bool isConfigured();
    
    // WiFi bağlantı durumu callback
    void setOnConfigSaved(void (*callback)());

private:
    Preferences _prefs;
    DeviceConfig _config;
    
    WebServer* _server;
    DNSServer* _dnsServer;
    bool _apModeActive;
    unsigned long _apStartTime;
    
    void (*_onConfigSaved)() = nullptr;
    
    void setupWebServer();
    void handleRoot();
    void handleSave();
    void handleStatus();
    void handleReset();
    
    String generateHTML();
    String generateStatusJSON();
};

extern ConfigManager Config;

#endif // CONFIG_MANAGER_H
