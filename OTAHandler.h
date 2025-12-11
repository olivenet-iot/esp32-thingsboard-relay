#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "Config.h"
#include "StatusLED.h"

class OTAHandler {
public:
    OTAHandler();
    
    void begin();
    void loop();
    
    void setOnStart(void (*callback)());
    void setOnEnd(void (*callback)());
    void setOnProgress(void (*callback)(unsigned int, unsigned int));
    void setOnError(void (*callback)(ota_error_t));

private:
    void (*_onStart)() = nullptr;
    void (*_onEnd)() = nullptr;
    void (*_onProgress)(unsigned int, unsigned int) = nullptr;
    void (*_onError)(ota_error_t) = nullptr;
};

extern OTAHandler OTA;

#endif // OTA_HANDLER_H
