#include "StatusLED.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

StatusLED Led;

StatusLED::StatusLED() {
    _currentStatus = LedStatus::OFF;
    _currentColor = LED_COLOR_OFF;
    _blinkInterval = 0;
    _blinkState = false;
    _lastBlinkTime = 0;
    _flashActive = false;
}

void StatusLED::begin() {
    pinMode(GPIO_RGB_LED, OUTPUT);
    writeColor(LED_COLOR_OFF);
    DEBUG_PRINTLN("[LED] Status LED initialized");
}

void StatusLED::update() {
    unsigned long now = millis();
    
    // Flash kontrolü
    if (_flashActive) {
        if (now >= _flashEndTime) {
            _flashActive = false;
            writeColor(_preFlashColor);
        }
        return;
    }
    
    // Blink kontrolü
    if (_blinkInterval > 0) {
        if (now - _lastBlinkTime >= _blinkInterval) {
            _lastBlinkTime = now;
            _blinkState = !_blinkState;
            writeColor(_blinkState ? _currentColor : LED_COLOR_OFF);
        }
    }
}

void StatusLED::setStatus(LedStatus status) {
    _currentStatus = status;
    _blinkInterval = 0;
    
    switch (status) {
        case LedStatus::OFF:
            writeColor(LED_COLOR_OFF);
            break;
            
        case LedStatus::BOOT:
            writeColor(LED_COLOR_WHITE);
            break;
            
        case LedStatus::AP_MODE:
            blink(LED_COLOR_BLUE, 500);
            break;
            
        case LedStatus::WIFI_CONNECTING:
            blink(LED_COLOR_YELLOW, 300);
            break;
            
        case LedStatus::MQTT_CONNECTING:
            blink(LED_COLOR_CYAN, 300);
            break;
            
        case LedStatus::CONNECTED:
            writeColor(LED_COLOR_GREEN);
            break;
            
        case LedStatus::ERROR:
            blink(LED_COLOR_RED, 200);
            break;
            
        case LedStatus::OTA_UPDATE:
            blink(LED_COLOR_PURPLE, 100);
            break;
    }
    
    DEBUG_PRINTF("[LED] Status changed to %d\n", (int)status);
}

void StatusLED::setColor(uint32_t color) {
    _currentColor = color;
    _blinkInterval = 0;
    writeColor(color);
}

void StatusLED::blink(uint32_t color, uint16_t intervalMs) {
    _currentColor = color;
    _blinkInterval = intervalMs;
    _blinkState = true;
    _lastBlinkTime = millis();
    writeColor(color);
}

void StatusLED::off() {
    setStatus(LedStatus::OFF);
}

void StatusLED::flash(uint32_t color, uint16_t durationMs) {
    _preFlashColor = _currentColor;
    _flashColor = color;
    _flashActive = true;
    _flashEndTime = millis() + durationMs;
    writeColor(color);
}

void StatusLED::writeColor(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Parlaklık ayarı (opsiyonel - %25)
    r = r >> 2;
    g = g >> 2;
    b = b >> 2;
    
    sendWS2812(r, g, b);
}

// WS2812 bit-banging (ESP32-S3 için - basitleştirilmiş)
// Not: Daha güvenilir sonuç için Adafruit_NeoPixel kütüphanesi önerilir
void StatusLED::sendWS2812(uint8_t r, uint8_t g, uint8_t b) {
    // WS2812 GRB formatı kullanır
    uint8_t data[3] = {g, r, b};
    
    portDISABLE_INTERRUPTS();
    
    for (int i = 0; i < 3; i++) {
        uint8_t byte = data[i];
        for (int bit = 7; bit >= 0; bit--) {
            if (byte & (1 << bit)) {
                // Bit 1: HIGH ~800ns, LOW ~450ns
                digitalWrite(GPIO_RGB_LED, HIGH);
                delayMicroseconds(1);
                digitalWrite(GPIO_RGB_LED, LOW);
                delayMicroseconds(0);
            } else {
                // Bit 0: HIGH ~400ns, LOW ~850ns  
                digitalWrite(GPIO_RGB_LED, HIGH);
                asm volatile("nop; nop; nop;");
                digitalWrite(GPIO_RGB_LED, LOW);
                delayMicroseconds(1);
            }
        }
    }
    
    portENABLE_INTERRUPTS();
    delayMicroseconds(50); // Reset
}
