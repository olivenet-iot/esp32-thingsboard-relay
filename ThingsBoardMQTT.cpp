#include "ThingsBoardMQTT.h"

ThingsBoardMQTT TB;
ThingsBoardMQTT* ThingsBoardMQTT::_instance = nullptr;

ThingsBoardMQTT::ThingsBoardMQTT() : _mqttClient(_wifiClient) {
    _lastTelemetryTime = 0;
    _lastReconnectAttempt = 0;
    _instance = this;
}

void ThingsBoardMQTT::begin() {
    DEBUG_PRINTLN("[TB] Initializing ThingsBoard MQTT...");
    
    DeviceConfig& cfg = Config.getConfig();
    
    _mqttClient.setServer(cfg.tbServer, cfg.tbPort);
    _mqttClient.setCallback(staticCallback);
    _mqttClient.setBufferSize(512);
    
    DEBUG_PRINTF("[TB] Server: %s:%d\n", cfg.tbServer, cfg.tbPort);
}

void ThingsBoardMQTT::loop() {
    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > MQTT_RECONNECT_DELAY_MS) {
            _lastReconnectAttempt = now;
            DEBUG_PRINTLN("[TB] Attempting MQTT reconnect...");
            if (connect()) {
                _lastReconnectAttempt = 0;
            }
        }
    } else {
        _mqttClient.loop();
        
        // Periyodik telemetry
        unsigned long now = millis();
        if (now - _lastTelemetryTime > TELEMETRY_INTERVAL_MS) {
            _lastTelemetryTime = now;
            sendTelemetry();
            sendAttributes();
        }
    }
}

bool ThingsBoardMQTT::connect() {
    DeviceConfig& cfg = Config.getConfig();
    
    if (strlen(cfg.tbToken) == 0) {
        DEBUG_PRINTLN("[TB] No access token configured");
        return false;
    }
    
    DEBUG_PRINTF("[TB] Connecting as %s...\n", cfg.tbToken);
    
    // ThingsBoard: username = access token, password = null
    String clientId = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    if (_mqttClient.connect(clientId.c_str(), cfg.tbToken, NULL)) {
        DEBUG_PRINTLN("[TB] Connected!");
        
        // RPC request topic'ine subscribe ol
        _mqttClient.subscribe(TB_RPC_REQUEST_TOPIC);
        DEBUG_PRINTLN("[TB] Subscribed to RPC requests");
        
        // İlk telemetry gönder
        sendTelemetry();
        sendAttributes();
        
        return true;
    } else {
        DEBUG_PRINTF("[TB] Connection failed, rc=%d\n", _mqttClient.state());
        return false;
    }
}

void ThingsBoardMQTT::disconnect() {
    if (_mqttClient.connected()) {
        _mqttClient.disconnect();
        DEBUG_PRINTLN("[TB] Disconnected");
    }
}

bool ThingsBoardMQTT::isConnected() {
    return _mqttClient.connected();
}

void ThingsBoardMQTT::sendTelemetry() {
    if (!_mqttClient.connected()) return;
    
    String payload = Relays.getStatesJson();
    
    if (_mqttClient.publish(TB_TELEMETRY_TOPIC, payload.c_str())) {
        DEBUG_PRINTF("[TB] Telemetry sent: %s\n", payload.c_str());
    } else {
        DEBUG_PRINTLN("[TB] Telemetry send failed");
    }
}

void ThingsBoardMQTT::sendTelemetry(const String& key, const String& value) {
    if (!_mqttClient.connected()) return;
    
    String payload = "{\"" + key + "\":\"" + value + "\"}";
    _mqttClient.publish(TB_TELEMETRY_TOPIC, payload.c_str());
}

void ThingsBoardMQTT::sendTelemetry(const String& key, float value) {
    if (!_mqttClient.connected()) return;
    
    String payload = "{\"" + key + "\":" + String(value, 2) + "}";
    _mqttClient.publish(TB_TELEMETRY_TOPIC, payload.c_str());
}

void ThingsBoardMQTT::sendTelemetry(const String& key, bool value) {
    if (!_mqttClient.connected()) return;
    
    String payload = "{\"" + key + "\":" + (value ? "true" : "false") + "}";
    _mqttClient.publish(TB_TELEMETRY_TOPIC, payload.c_str());
}

void ThingsBoardMQTT::sendAttributes() {
    if (!_mqttClient.connected()) return;
    
    StaticJsonDocument<256> doc;
    doc["firmware"] = FIRMWARE_VERSION;
    doc["device_type"] = DEVICE_TYPE;
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    
    String payload;
    serializeJson(doc, payload);
    
    if (_mqttClient.publish(TB_ATTRIBUTES_TOPIC, payload.c_str())) {
        DEBUG_PRINTF("[TB] Attributes sent: %s\n", payload.c_str());
    }
}

void ThingsBoardMQTT::sendAttribute(const String& key, const String& value) {
    if (!_mqttClient.connected()) return;
    
    String payload = "{\"" + key + "\":\"" + value + "\"}";
    _mqttClient.publish(TB_ATTRIBUTES_TOPIC, payload.c_str());
}

bool ThingsBoardMQTT::publish(const char* topic, const char* payload) {
    return _mqttClient.publish(topic, payload);
}

void ThingsBoardMQTT::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->onMessage(topic, payload, length);
    }
}

void ThingsBoardMQTT::onMessage(char* topic, byte* payload, unsigned int length) {
    DEBUG_PRINTF("[TB] Message received on %s\n", topic);
    
    // Null-terminate payload
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    DEBUG_PRINTF("[TB] Payload: %s\n", message);
    
    String topicStr = String(topic);
    
    // RPC Request: v1/devices/me/rpc/request/{requestId}
    if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
        int requestId = topicStr.substring(26).toInt();
        
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, message);
        
        if (error) {
            DEBUG_PRINTF("[TB] JSON parse error: %s\n", error.c_str());
            return;
        }
        
        handleRPCRequest(requestId, doc);
    }
}

void ThingsBoardMQTT::handleRPCRequest(int requestId, JsonDocument& doc) {
    String method = doc["method"].as<String>();
    
    DEBUG_PRINTF("[TB] RPC method: %s, requestId: %d\n", method.c_str(), requestId);
    
    String response;
    
    // ========== setRelay ==========
    // {"method":"setRelay","params":{"relay":1,"state":true}}
    if (method == "setRelay") {
        int relay = doc["params"]["relay"] | 0;
        bool state = doc["params"]["state"] | false;
        
        if (relay >= 1 && relay <= RELAY_COUNT) {
            Relays.setState(relay, state);
            response = "{\"relay" + String(relay) + "\":" + (Relays.getState(relay) ? "true" : "false") + "}";
            
            // Telemetry güncelle
            sendTelemetry();
        } else {
            response = "{\"error\":\"Invalid relay number\"}";
        }
    }
    // ========== toggleRelay ==========
    // {"method":"toggleRelay","params":{"relay":1}}
    else if (method == "toggleRelay") {
        int relay = doc["params"]["relay"] | 0;
        
        if (relay >= 1 && relay <= RELAY_COUNT) {
            Relays.toggle(relay);
            response = "{\"relay" + String(relay) + "\":" + (Relays.getState(relay) ? "true" : "false") + "}";
            
            sendTelemetry();
        } else {
            response = "{\"error\":\"Invalid relay number\"}";
        }
    }
    // ========== setAllRelays ==========
    // {"method":"setAllRelays","params":{"state":true}}
    else if (method == "setAllRelays") {
        bool state = doc["params"]["state"] | false;
        Relays.setAll(state);
        response = Relays.getStatesJson();
        
        sendTelemetry();
    }
    // ========== getRelayStates ==========
    // {"method":"getRelayStates","params":{}}
    else if (method == "getRelayStates") {
        response = Relays.getStatesJson();
    }
    // ========== getDeviceInfo ==========
    // {"method":"getDeviceInfo","params":{}}
    else if (method == "getDeviceInfo") {
        StaticJsonDocument<256> info;
        info["firmware"] = FIRMWARE_VERSION;
        info["device_type"] = DEVICE_TYPE;
        info["ip"] = WiFi.localIP().toString();
        info["mac"] = WiFi.macAddress();
        info["rssi"] = WiFi.RSSI();
        info["uptime"] = millis() / 1000;
        info["free_heap"] = ESP.getFreeHeap();
        info["relay_count"] = RELAY_COUNT;
        serializeJson(info, response);
    }
    // ========== reboot ==========
    // {"method":"reboot","params":{}}
    else if (method == "reboot") {
        response = "{\"status\":\"rebooting\"}";
        sendRPCResponse(requestId, response);
        delay(500);
        ESP.restart();
        return;
    }
    // ========== resetConfig ==========
    // {"method":"resetConfig","params":{}}
    else if (method == "resetConfig") {
        response = "{\"status\":\"resetting\"}";
        sendRPCResponse(requestId, response);
        delay(500);
        Config.resetConfig();
        ESP.restart();
        return;
    }
    // ========== Unknown method ==========
    else {
        response = "{\"error\":\"Unknown method: " + method + "\"}";
    }
    
    sendRPCResponse(requestId, response);
}

void ThingsBoardMQTT::sendRPCResponse(int requestId, const String& response) {
    String topic = String(TB_RPC_RESPONSE_TOPIC) + String(requestId);
    
    if (_mqttClient.publish(topic.c_str(), response.c_str())) {
        DEBUG_PRINTF("[TB] RPC response sent to %s: %s\n", topic.c_str(), response.c_str());
    } else {
        DEBUG_PRINTLN("[TB] RPC response send failed");
    }
}
