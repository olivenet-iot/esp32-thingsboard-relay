# ESP32-S3-Relay-6CH ThingsBoard Firmware

Production-ready firmware for Waveshare ESP32-S3-Relay-6CH industrial relay module with ThingsBoard integration.

## Features

- **WiFi Configuration Portal**: AP mode captive portal for easy setup
- **ThingsBoard MQTT**: Full RPC and telemetry support
- **6-Channel Relay Control**: Individual and bulk control
- **OTA Updates**: Over-the-air firmware updates via Arduino IDE
- **RGB LED Status**: Visual feedback for all states
- **Buzzer Feedback**: Audio feedback for operations
- **Watchdog Timer**: Auto-recovery from crashes
- **Auto-Reconnect**: Automatic WiFi and MQTT reconnection

## Hardware

- **Module**: Waveshare ESP32-S3-Relay-6CH
- **MCU**: ESP32-S3 (Xtensa LX7 dual-core, 240MHz)
- **Relays**: 6x 10A 250VAC/30VDC

### Pin Mapping

| GPIO | Function |
|------|----------|
| 1 | Relay 1 |
| 2 | Relay 2 |
| 41 | Relay 3 |
| 42 | Relay 4 |
| 45 | Relay 5 |
| 46 | Relay 6 |
| 38 | WS2812 RGB LED |
| 21 | Buzzer (PWM) |

## Installation

### 1. Arduino IDE Setup

1. Install Arduino IDE 2.x
2. Add ESP32 board support:
   - File → Preferences → Additional Board Manager URLs:
   - `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Tools → Board → Boards Manager → Search "esp32" → Install
4. Install required libraries:
   - **PubSubClient** (by Nick O'Leary)
   - **ArduinoJson** (by Benoit Blanchon)

### 2. Board Configuration

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | Default 4MB with spiffs |
| PSRAM | OPI PSRAM |

### 3. Upload

1. Open `ESP32_TB_Relay.ino`
2. Connect USB-C cable
3. Select correct COM port
4. Click Upload

## First Boot Setup

1. Power on the device
2. Connect to WiFi network: `ESP32-Relay-Setup`
3. Password: `12345678`
4. Browser will open automatically (or go to `192.168.4.1`)
5. Enter:
   - WiFi SSID and password
   - ThingsBoard server address
   - ThingsBoard port (default: 1883)
   - Device Access Token
6. Click "Save and Connect"
7. Device will restart and connect

## LED Status

| Color | Pattern | Meaning |
|-------|---------|---------|
| White | Solid | Booting |
| Blue | Blinking | AP Mode (setup) |
| Yellow | Blinking | WiFi connecting |
| Cyan | Blinking | MQTT connecting |
| Green | Solid | Connected |
| Red | Blinking | Error |
| Purple | Blinking | OTA update |

## ThingsBoard Integration

### Telemetry (Auto-sent)

```json
{
  "relay1": true,
  "relay2": false,
  "relay3": true,
  "relay4": false,
  "relay5": false,
  "relay6": false
}
```

### Attributes (Auto-sent)

```json
{
  "firmware": "1.0.0",
  "device_type": "ESP32-S3-Relay-6CH",
  "ip": "192.168.1.100",
  "mac": "AA:BB:CC:DD:EE:FF",
  "rssi": -65,
  "uptime": 3600,
  "free_heap": 250000
}
```

### RPC Commands

#### setRelay
Control single relay:
```json
{
  "method": "setRelay",
  "params": {
    "relay": 1,
    "state": true
  }
}
```

#### toggleRelay
Toggle single relay:
```json
{
  "method": "toggleRelay",
  "params": {
    "relay": 1
  }
}
```

#### setAllRelays
Control all relays:
```json
{
  "method": "setAllRelays",
  "params": {
    "state": true
  }
}
```

#### getRelayStates
Get current states:
```json
{
  "method": "getRelayStates",
  "params": {}
}
```

#### getDeviceInfo
Get device information:
```json
{
  "method": "getDeviceInfo",
  "params": {}
}
```

#### reboot
Restart device:
```json
{
  "method": "reboot",
  "params": {}
}
```

#### resetConfig
Factory reset:
```json
{
  "method": "resetConfig",
  "params": {}
}
```

## ThingsBoard Dashboard Widget Examples

### Switch Widget (for each relay)
- Widget Type: Control widgets → Switch
- Target: Device
- RPC method: `setRelay`
- Request body: `{"relay": 1, "state": ${value}}`

### LED Indicator Widget
- Widget Type: Cards → Value card
- Data key: `relay1`
- Value format: `${value ? 'ON' : 'OFF'}`

## OTA Updates

1. Ensure device is connected to same network as your computer
2. In Arduino IDE: Tools → Port → Select network port (ESP32-Relay-XXXXXX)
3. Upload as normal

## Troubleshooting

### Can't connect to AP mode
- Hold BOOT button while pressing RESET
- Wait for blue blinking LED

### WiFi won't connect
- Check SSID and password
- Ensure 2.4GHz network (5GHz not supported)
- Check router allows new connections

### MQTT won't connect
- Verify ThingsBoard server address
- Check Access Token is correct
- Ensure port 1883 is not blocked

### Factory Reset
- Hold BOOT button for 10 seconds, or
- Send `resetConfig` RPC command, or
- Use web portal reset button

## File Structure

```
ESP32_TB_Relay/
├── ESP32_TB_Relay.ino    # Main firmware
├── Config.h              # Configuration constants
├── RelayController.h/cpp # Relay control class
├── StatusLED.h/cpp       # RGB LED status
├── ConfigManager.h/cpp   # WiFi/NVS configuration
├── ThingsBoardMQTT.h/cpp # ThingsBoard MQTT client
├── OTAHandler.h/cpp      # OTA update handler
├── Buzzer.h/cpp          # Buzzer control
└── README.md             # This file
```

## License

MIT License - Feel free to use and modify.

## Support

For issues and feature requests, contact Olivenet Ltd.
