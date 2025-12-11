#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"

class RelayController {
public:
    RelayController();
    
    void begin();
    
    // Röle kontrol
    bool setState(uint8_t channel, bool state);
    bool toggle(uint8_t channel);
    bool getState(uint8_t channel);
    
    // Toplu işlemler
    void setAll(bool state);
    void toggleAll();
    
    // Durum sorgulama
    String getStatesJson();
    uint8_t getStatesBitmask();
    
    // Callback (durum değiştiğinde çağrılır)
    void setOnChangeCallback(void (*callback)(uint8_t channel, bool state));

private:
    bool _states[RELAY_COUNT];
    uint8_t _pins[RELAY_COUNT] = {
        GPIO_RELAY_1, GPIO_RELAY_2, GPIO_RELAY_3,
        GPIO_RELAY_4, GPIO_RELAY_5, GPIO_RELAY_6
    };
    void (*_onChangeCallback)(uint8_t channel, bool state) = nullptr;
    
    void applyState(uint8_t channel);
    void notifyChange(uint8_t channel);
};

extern RelayController Relays;

#endif // RELAY_CONTROLLER_H
