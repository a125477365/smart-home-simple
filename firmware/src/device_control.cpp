/**
 * 设备控制模块实现
 */
#include "device_control.h"
#include "config.h"
#include "config_store.h"

DeviceControl& DeviceControl::instance() {
    static DeviceControl _instance;
    return _instance;
}

void DeviceControl::begin() {
    if (!initialized) {
        pinMode(RELAY_PIN, OUTPUT);
        pinMode(LED_PIN, OUTPUT);
        
        // 恢复上次状态
        restoreState();
        
        initialized = true;
        Serial.println("DeviceControl initialized");
    }
}

void DeviceControl::setRelay(bool state) {
    relayState = state;
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
    Serial.printf("Relay: %s\n", state ? "ON" : "OFF");
}

void DeviceControl::toggleRelay() {
    setRelay(!relayState);
}

void DeviceControl::setBrightness(uint8_t value) {
    brightness = constrain(value, 0, 100);
    
    // 如果支持 PWM 调光，这里实现
    // 目前简单实现：brightness > 0 时才允许开灯
    if (brightness == 0) {
        setRelay(false);
    }
    
    Serial.printf("Brightness: %d%%\n", brightness);
}

void DeviceControl::saveState() {
    ConfigStore::instance().saveRelayState(relayState);
    ConfigStore::instance().saveBrightness(brightness);
    Serial.println("State saved");
}

void DeviceControl::restoreState() {
    relayState = ConfigStore::instance().loadRelayState();
    brightness = ConfigStore::instance().loadBrightness();
    
    // 应用状态
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.printf("State restored: relay=%s, brightness=%d%%\n", 
                  relayState ? "ON" : "OFF", brightness);
}
