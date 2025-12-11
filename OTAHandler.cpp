#include "OTAHandler.h"

OTAHandler OTA;

OTAHandler::OTAHandler() {
}

void OTAHandler::begin() {
    DEBUG_PRINTLN("[OTA] Initializing...");
    
    // Hostname ayarla
    String hostname = "ESP32-Relay-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    ArduinoOTA.setHostname(hostname.c_str());
    
    // Şifre (opsiyonel - güvenlik için)
    // ArduinoOTA.setPassword("admin");
    
    ArduinoOTA.onStart([this]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "firmware";
        } else {
            type = "filesystem";
        }
        DEBUG_PRINTF("[OTA] Start updating %s\n", type.c_str());
        Led.setStatus(LedStatus::OTA_UPDATE);
        
        if (_onStart) _onStart();
    });
    
    ArduinoOTA.onEnd([this]() {
        DEBUG_PRINTLN("\n[OTA] Update complete!");
        Led.setColor(LED_COLOR_GREEN);
        
        if (_onEnd) _onEnd();
    });
    
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        unsigned int percent = (progress / (total / 100));
        DEBUG_PRINTF("[OTA] Progress: %u%%\r", percent);
        
        if (_onProgress) _onProgress(progress, total);
    });
    
    ArduinoOTA.onError([this](ota_error_t error) {
        DEBUG_PRINTF("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
        else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
        
        Led.setStatus(LedStatus::ERROR);
        
        if (_onError) _onError(error);
    });
    
    ArduinoOTA.begin();
    
    DEBUG_PRINTF("[OTA] Ready - Hostname: %s\n", hostname.c_str());
}

void OTAHandler::loop() {
    ArduinoOTA.handle();
}

void OTAHandler::setOnStart(void (*callback)()) {
    _onStart = callback;
}

void OTAHandler::setOnEnd(void (*callback)()) {
    _onEnd = callback;
}

void OTAHandler::setOnProgress(void (*callback)(unsigned int, unsigned int)) {
    _onProgress = callback;
}

void OTAHandler::setOnError(void (*callback)(ota_error_t)) {
    _onError = callback;
}
