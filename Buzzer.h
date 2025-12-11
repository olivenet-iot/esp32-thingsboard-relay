#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include "Config.h"

class Buzzer {
public:
    Buzzer();
    
    void begin();
    
    // Basit sesler
    void beep(uint16_t durationMs = 100);
    void beep(uint16_t frequency, uint16_t durationMs);
    void doubleBeep();
    void tripleBeep();
    
    // Durum sesleri
    void bootSound();
    void successSound();
    void errorSound();
    void clickSound();
    
    // Sessiz mod
    void setMuted(bool muted);
    bool isMuted();

private:
    bool _muted;
    
    void tone(uint16_t frequency, uint16_t durationMs);
    void noTone();
};

extern Buzzer Buzz;

#endif // BUZZER_H
