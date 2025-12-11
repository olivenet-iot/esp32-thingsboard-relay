#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>
#include "Config.h"

// LED durumları
enum class LedStatus {
    OFF,
    BOOT,           // Beyaz - açılış
    AP_MODE,        // Mavi yanıp söner - AP mode
    WIFI_CONNECTING,// Sarı yanıp söner - WiFi bağlanıyor
    MQTT_CONNECTING,// Cyan yanıp söner - MQTT bağlanıyor
    CONNECTED,      // Yeşil sabit - tam bağlı
    ERROR,          // Kırmızı yanıp söner - hata
    OTA_UPDATE      // Mor yanıp söner - OTA güncelleme
};

class StatusLED {
public:
    StatusLED();
    
    void begin();
    void update();  // loop() içinde çağrılmalı
    
    void setStatus(LedStatus status);
    void setColor(uint32_t color);
    void blink(uint32_t color, uint16_t intervalMs);
    void off();
    
    // Kısa bildirimler
    void flash(uint32_t color, uint16_t durationMs);

private:
    LedStatus _currentStatus;
    uint32_t _currentColor;
    uint16_t _blinkInterval;
    bool _blinkState;
    unsigned long _lastBlinkTime;
    
    bool _flashActive;
    uint32_t _flashColor;
    unsigned long _flashEndTime;
    uint32_t _preFlashColor;
    
    void writeColor(uint32_t color);
    void sendWS2812(uint8_t r, uint8_t g, uint8_t b);
};

extern StatusLED Led;

#endif // STATUS_LED_H
