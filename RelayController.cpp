#include "RelayController.h"

RelayController Relays;

RelayController::RelayController() {
    for (int i = 0; i < RELAY_COUNT; i++) {
        _states[i] = false;
    }
}

void RelayController::begin() {
    DEBUG_PRINTLN("[Relay] Initializing relays...");
    
    for (int i = 0; i < RELAY_COUNT; i++) {
        pinMode(_pins[i], OUTPUT);
        digitalWrite(_pins[i], RELAY_OFF);
        _states[i] = false;
        DEBUG_PRINTF("[Relay] CH%d -> GPIO%d initialized\n", i + 1, _pins[i]);
    }
    
    DEBUG_PRINTLN("[Relay] All relays initialized OFF");
}

bool RelayController::setState(uint8_t channel, bool state) {
    if (channel < 1 || channel > RELAY_COUNT) {
        DEBUG_PRINTF("[Relay] Invalid channel: %d\n", channel);
        return false;
    }
    
    uint8_t idx = channel - 1;
    
    if (_states[idx] != state) {
        _states[idx] = state;
        applyState(idx);
        notifyChange(channel);
        DEBUG_PRINTF("[Relay] CH%d set to %s\n", channel, state ? "ON" : "OFF");
    }
    
    return true;
}

bool RelayController::toggle(uint8_t channel) {
    if (channel < 1 || channel > RELAY_COUNT) {
        DEBUG_PRINTF("[Relay] Invalid channel: %d\n", channel);
        return false;
    }
    
    uint8_t idx = channel - 1;
    _states[idx] = !_states[idx];
    applyState(idx);
    notifyChange(channel);
    
    DEBUG_PRINTF("[Relay] CH%d toggled to %s\n", channel, _states[idx] ? "ON" : "OFF");
    return true;
}

bool RelayController::getState(uint8_t channel) {
    if (channel < 1 || channel > RELAY_COUNT) {
        return false;
    }
    return _states[channel - 1];
}

void RelayController::setAll(bool state) {
    DEBUG_PRINTF("[Relay] Setting ALL relays to %s\n", state ? "ON" : "OFF");
    
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (_states[i] != state) {
            _states[i] = state;
            applyState(i);
            notifyChange(i + 1);
        }
    }
}

void RelayController::toggleAll() {
    DEBUG_PRINTLN("[Relay] Toggling ALL relays");
    
    for (int i = 0; i < RELAY_COUNT; i++) {
        _states[i] = !_states[i];
        applyState(i);
        notifyChange(i + 1);
    }
}

String RelayController::getStatesJson() {
    String json = "{";
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (i > 0) json += ",";
        json += "\"relay" + String(i + 1) + "\":" + (_states[i] ? "true" : "false");
    }
    json += "}";
    return json;
}

uint8_t RelayController::getStatesBitmask() {
    uint8_t mask = 0;
    for (int i = 0; i < RELAY_COUNT; i++) {
        if (_states[i]) {
            mask |= (1 << i);
        }
    }
    return mask;
}

void RelayController::setOnChangeCallback(void (*callback)(uint8_t channel, bool state)) {
    _onChangeCallback = callback;
}

void RelayController::applyState(uint8_t idx) {
    digitalWrite(_pins[idx], _states[idx] ? RELAY_ON : RELAY_OFF);
}

void RelayController::notifyChange(uint8_t channel) {
    if (_onChangeCallback != nullptr) {
        _onChangeCallback(channel, _states[channel - 1]);
    }
}
