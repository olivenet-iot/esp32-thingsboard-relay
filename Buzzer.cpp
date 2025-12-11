#include "Buzzer.h"

Buzzer Buzz;

Buzzer::Buzzer() {
    _muted = false;
}

void Buzzer::begin() {
    DEBUG_PRINTLN("[Buzzer] Initializing...");
    
    // ESP32 Arduino Core 3.x API
    ledcAttach(GPIO_BUZZER, 2000, 8);
    ledcWrite(GPIO_BUZZER, 0);
    
    DEBUG_PRINTLN("[Buzzer] Ready");
}

void Buzzer::beep(uint16_t durationMs) {
    beep(2000, durationMs);
}

void Buzzer::beep(uint16_t frequency, uint16_t durationMs) {
    if (_muted) return;
    
    tone(frequency, durationMs);
}

void Buzzer::doubleBeep() {
    if (_muted) return;
    
    beep(2000, 50);
    delay(50);
    beep(2000, 50);
}

void Buzzer::tripleBeep() {
    if (_muted) return;
    
    beep(2000, 50);
    delay(50);
    beep(2000, 50);
    delay(50);
    beep(2000, 50);
}

void Buzzer::bootSound() {
    if (_muted) return;
    
    // Artan ton - açılış
    beep(1000, 100);
    delay(50);
    beep(1500, 100);
    delay(50);
    beep(2000, 150);
}

void Buzzer::successSound() {
    if (_muted) return;
    
    // İki yükselen ton
    beep(1500, 100);
    delay(50);
    beep(2500, 150);
}

void Buzzer::errorSound() {
    if (_muted) return;
    
    // Düşük ton, uzun
    beep(500, 300);
    delay(100);
    beep(500, 300);
}

void Buzzer::clickSound() {
    if (_muted) return;
    
    beep(3000, 10);
}

void Buzzer::setMuted(bool muted) {
    _muted = muted;
    DEBUG_PRINTF("[Buzzer] Muted: %s\n", muted ? "yes" : "no");
}

bool Buzzer::isMuted() {
    return _muted;
}

void Buzzer::tone(uint16_t frequency, uint16_t durationMs) {
    ledcWriteTone(GPIO_BUZZER, frequency);
    delay(durationMs);
    noTone();
}

void Buzzer::noTone() {
    ledcWriteTone(GPIO_BUZZER, 0);
}
