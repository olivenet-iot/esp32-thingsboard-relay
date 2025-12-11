#include "ConfigManager.h"

ConfigManager Config;

ConfigManager::ConfigManager() {
    _server = nullptr;
    _dnsServer = nullptr;
    _apModeActive = false;
    memset(&_config, 0, sizeof(_config));
    _config.tbPort = TB_PORT_DEFAULT;
}

void ConfigManager::begin() {
    DEBUG_PRINTLN("[Config] Initializing...");
    loadConfig();
}

bool ConfigManager::loadConfig() {
    DEBUG_PRINTLN("[Config] Loading from NVS...");
    
    _prefs.begin(NVS_NAMESPACE, true); // read-only
    
    _config.configured = _prefs.getBool(NVS_KEY_CONFIGURED, false);
    
    if (_config.configured) {
        String ssid = _prefs.getString(NVS_KEY_WIFI_SSID, "");
        String pass = _prefs.getString(NVS_KEY_WIFI_PASS, "");
        String server = _prefs.getString(NVS_KEY_TB_SERVER, "");
        String token = _prefs.getString(NVS_KEY_TB_TOKEN, "");
        _config.tbPort = _prefs.getUShort(NVS_KEY_TB_PORT, TB_PORT_DEFAULT);
        
        strncpy(_config.wifiSsid, ssid.c_str(), sizeof(_config.wifiSsid) - 1);
        strncpy(_config.wifiPassword, pass.c_str(), sizeof(_config.wifiPassword) - 1);
        strncpy(_config.tbServer, server.c_str(), sizeof(_config.tbServer) - 1);
        strncpy(_config.tbToken, token.c_str(), sizeof(_config.tbToken) - 1);
        
        DEBUG_PRINTF("[Config] Loaded - SSID: %s, Server: %s\n", _config.wifiSsid, _config.tbServer);
    } else {
        DEBUG_PRINTLN("[Config] Not configured yet");
    }
    
    _prefs.end();
    return _config.configured;
}

bool ConfigManager::saveConfig() {
    DEBUG_PRINTLN("[Config] Saving to NVS...");
    
    _prefs.begin(NVS_NAMESPACE, false); // read-write
    
    _prefs.putString(NVS_KEY_WIFI_SSID, _config.wifiSsid);
    _prefs.putString(NVS_KEY_WIFI_PASS, _config.wifiPassword);
    _prefs.putString(NVS_KEY_TB_SERVER, _config.tbServer);
    _prefs.putUShort(NVS_KEY_TB_PORT, _config.tbPort);
    _prefs.putString(NVS_KEY_TB_TOKEN, _config.tbToken);
    _prefs.putBool(NVS_KEY_CONFIGURED, true);
    
    _config.configured = true;
    
    _prefs.end();
    
    DEBUG_PRINTLN("[Config] Saved successfully");
    return true;
}

void ConfigManager::resetConfig() {
    DEBUG_PRINTLN("[Config] Resetting...");
    
    _prefs.begin(NVS_NAMESPACE, false);
    _prefs.clear();
    _prefs.end();
    
    memset(&_config, 0, sizeof(_config));
    _config.tbPort = TB_PORT_DEFAULT;
    _config.configured = false;
    
    DEBUG_PRINTLN("[Config] Reset complete");
}

void ConfigManager::startAPMode() {
    DEBUG_PRINTLN("[Config] Starting AP Mode...");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    IPAddress apIP = WiFi.softAPIP();
    DEBUG_PRINTF("[Config] AP IP: %s\n", apIP.toString().c_str());
    
    // DNS Server - tüm domain'leri yakala (captive portal)
    _dnsServer = new DNSServer();
    _dnsServer->start(53, "*", apIP);
    
    // Web Server
    _server = new WebServer(80);
    setupWebServer();
    _server->begin();
    
    _apModeActive = true;
    _apStartTime = millis();
    
    DEBUG_PRINTF("[Config] AP Mode active - SSID: %s, Pass: %s\n", AP_SSID, AP_PASSWORD);
}

void ConfigManager::stopAPMode() {
    if (!_apModeActive) return;
    
    DEBUG_PRINTLN("[Config] Stopping AP Mode...");
    
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    
    WiFi.softAPdisconnect(true);
    _apModeActive = false;
    
    DEBUG_PRINTLN("[Config] AP Mode stopped");
}

void ConfigManager::handlePortal() {
    if (!_apModeActive) return;
    
    _dnsServer->processNextRequest();
    _server->handleClient();
    
    // Timeout kontrolü
    if (millis() - _apStartTime > AP_TIMEOUT_MS) {
        DEBUG_PRINTLN("[Config] AP Mode timeout");
        stopAPMode();
    }
}

bool ConfigManager::isAPModeActive() {
    return _apModeActive;
}

DeviceConfig& ConfigManager::getConfig() {
    return _config;
}

bool ConfigManager::isConfigured() {
    return _config.configured;
}

void ConfigManager::setOnConfigSaved(void (*callback)()) {
    _onConfigSaved = callback;
}

void ConfigManager::setupWebServer() {
    _server->on("/", HTTP_GET, [this]() { handleRoot(); });
    _server->on("/save", HTTP_POST, [this]() { handleSave(); });
    _server->on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server->on("/reset", HTTP_POST, [this]() { handleReset(); });
    _server->onNotFound([this]() { handleRoot(); }); // Captive portal
}

void ConfigManager::handleRoot() {
    _server->send(200, "text/html", generateHTML());
}

void ConfigManager::handleSave() {
    if (_server->hasArg("wifi_ssid") && _server->hasArg("tb_server") && _server->hasArg("tb_token")) {
        strncpy(_config.wifiSsid, _server->arg("wifi_ssid").c_str(), sizeof(_config.wifiSsid) - 1);
        strncpy(_config.wifiPassword, _server->arg("wifi_pass").c_str(), sizeof(_config.wifiPassword) - 1);
        strncpy(_config.tbServer, _server->arg("tb_server").c_str(), sizeof(_config.tbServer) - 1);
        _config.tbPort = _server->arg("tb_port").toInt();
        if (_config.tbPort == 0) _config.tbPort = TB_PORT_DEFAULT;
        strncpy(_config.tbToken, _server->arg("tb_token").c_str(), sizeof(_config.tbToken) - 1);
        
        saveConfig();
        
        String response = R"(
<!DOCTYPE html><html><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Kaydedildi</title>
<style>body{font-family:Arial;background:#1a1a2e;color:#fff;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}
.box{background:#16213e;padding:40px;border-radius:10px;text-align:center}
.success{color:#4ade80;font-size:48px;margin-bottom:20px}</style></head>
<body><div class="box"><div class="success">✓</div><h2>Ayarlar Kaydedildi!</h2>
<p>Cihaz 3 saniye içinde yeniden başlatılacak...</p></div></body></html>
)";
        _server->send(200, "text/html", response);
        
        delay(3000);
        
        if (_onConfigSaved) {
            _onConfigSaved();
        }
        
        ESP.restart();
    } else {
        _server->send(400, "text/plain", "Missing parameters");
    }
}

void ConfigManager::handleStatus() {
    _server->send(200, "application/json", generateStatusJSON());
}

void ConfigManager::handleReset() {
    resetConfig();
    _server->send(200, "text/plain", "Config reset. Restarting...");
    delay(1000);
    ESP.restart();
}

String ConfigManager::generateHTML() {
    String html = R"rawhtml(
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Relay Kurulum</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: #0f3460;
            border-radius: 16px;
            padding: 30px;
            width: 100%;
            max-width: 400px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        }
        h1 {
            color: #e94560;
            text-align: center;
            margin-bottom: 8px;
            font-size: 24px;
        }
        .subtitle {
            color: #888;
            text-align: center;
            margin-bottom: 25px;
            font-size: 14px;
        }
        .section {
            margin-bottom: 25px;
        }
        .section-title {
            color: #4ade80;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 12px;
            padding-bottom: 8px;
            border-bottom: 1px solid #1a1a2e;
        }
        label {
            display: block;
            color: #ccc;
            margin-bottom: 6px;
            font-size: 14px;
        }
        input {
            width: 100%;
            padding: 12px;
            border: 2px solid #1a1a2e;
            border-radius: 8px;
            background: #16213e;
            color: #fff;
            font-size: 14px;
            margin-bottom: 15px;
            transition: border-color 0.3s;
        }
        input:focus {
            outline: none;
            border-color: #e94560;
        }
        input::placeholder {
            color: #555;
        }
        .row {
            display: flex;
            gap: 10px;
        }
        .row > div {
            flex: 1;
        }
        .row > div:first-child {
            flex: 2;
        }
        button {
            width: 100%;
            padding: 14px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(0,0,0,0.3);
        }
        .btn-primary {
            background: linear-gradient(135deg, #e94560, #c73659);
            color: white;
        }
        .btn-secondary {
            background: #1a1a2e;
            color: #888;
            margin-top: 10px;
        }
        .info {
            background: #1a1a2e;
            border-radius: 8px;
            padding: 12px;
            margin-top: 20px;
            font-size: 12px;
            color: #666;
        }
        .info code {
            color: #4ade80;
        }
    </style>
    <script>
        function doReset() {
            if(confirm('Tum ayarlar silinecek. Emin misiniz?')) {
                fetch('/reset',{method:'POST'}).then(function(){location.reload()});
            }
        }
    </script>
</head>
<body>
    <div class="container">
        <h1>ESP32 Relay</h1>
        <p class="subtitle">ThingsBoard Konfigurasyonu</p>
        
        <form action="/save" method="POST">
            <div class="section">
                <div class="section-title">WiFi Ayarlari</div>
                <label>WiFi Ag Adi (SSID)</label>
                <input type="text" name="wifi_ssid" placeholder="WiFi ag adini girin" required 
                       value=")rawhtml";
    html += _config.wifiSsid;
    html += R"rawhtml(">
                <label>WiFi Sifresi</label>
                <input type="password" name="wifi_pass" placeholder="WiFi sifresini girin"
                       value=")rawhtml";
    html += _config.wifiPassword;
    html += R"rawhtml(">
            </div>
            
            <div class="section">
                <div class="section-title">ThingsBoard Ayarlari</div>
                <div class="row">
                    <div>
                        <label>Sunucu Adresi</label>
                        <input type="text" name="tb_server" placeholder="orn: tb.example.com" required
                               value=")rawhtml";
    html += _config.tbServer;
    html += R"rawhtml(">
                    </div>
                    <div>
                        <label>Port</label>
                        <input type="number" name="tb_port" placeholder="1883" 
                               value=")rawhtml";
    html += String(_config.tbPort > 0 ? _config.tbPort : TB_PORT_DEFAULT);
    html += R"rawhtml(">
                    </div>
                </div>
                <label>Access Token</label>
                <input type="text" name="tb_token" placeholder="Cihaz access token" required
                       value=")rawhtml";
    html += _config.tbToken;
    html += R"rawhtml(">
            </div>
            
            <button type="submit" class="btn-primary">Kaydet ve Baglan</button>
        </form>
        
        <button onclick="doReset()" class="btn-secondary">Fabrika Ayarlarina Don</button>
        
        <div class="info">
            <strong>Firmware:</strong> <code>v)rawhtml";
    html += FIRMWARE_VERSION;
    html += R"rawhtml(</code><br>
            <strong>MAC:</strong> <code>)rawhtml";
    html += WiFi.macAddress();
    html += R"rawhtml(</code>
        </div>
    </div>
</body>
</html>
)rawhtml";
    return html;
}

String ConfigManager::generateStatusJSON() {
    String json = "{";
    json += "\"configured\":" + String(_config.configured ? "true" : "false") + ",";
    json += "\"wifi_ssid\":\"" + String(_config.wifiSsid) + "\",";
    json += "\"tb_server\":\"" + String(_config.tbServer) + "\",";
    json += "\"tb_port\":" + String(_config.tbPort) + ",";
    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\"";
    json += "}";
    return json;
}
