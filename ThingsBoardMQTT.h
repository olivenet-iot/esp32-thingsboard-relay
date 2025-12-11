#ifndef THINGSBOARD_MQTT_H
#define THINGSBOARD_MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "ConfigManager.h"
#include "RelayController.h"

class ThingsBoardMQTT {
public:
    ThingsBoardMQTT();
    
    void begin();
    void loop();
    
    bool connect();
    void disconnect();
    bool isConnected();
    
    // Telemetry
    void sendTelemetry();
    void sendTelemetry(const String& key, const String& value);
    void sendTelemetry(const String& key, float value);
    void sendTelemetry(const String& key, bool value);
    
    // Attributes
    void sendAttributes();
    void sendAttribute(const String& key, const String& value);
    
    // Manuel publish
    bool publish(const char* topic, const char* payload);

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    unsigned long _lastTelemetryTime;
    unsigned long _lastReconnectAttempt;
    
    void setupCallbacks();
    void onMessage(char* topic, byte* payload, unsigned int length);
    void handleRPCRequest(int requestId, JsonDocument& doc);
    void sendRPCResponse(int requestId, const String& response);
    
    static ThingsBoardMQTT* _instance;
    static void staticCallback(char* topic, byte* payload, unsigned int length);
};

extern ThingsBoardMQTT TB;

#endif // THINGSBOARD_MQTT_H
