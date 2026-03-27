/**
 * 设备控制模块 - 继电器/调光
 */
#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>

class DeviceControl {
public:
    static DeviceControl& instance();
    
    void begin();
    
    // 继电器控制
    void setRelay(bool state);
    bool getRelayState() const { return relayState; }
    void toggleRelay();
    
    // 亮度控制（PWM）
    void setBrightness(uint8_t value);  // 0-100
    uint8_t getBrightness() const { return brightness; }
    
    // 状态保存/恢复
    void saveState();
    void restoreState();

private:
    DeviceControl() = default;
    bool relayState = false;
    uint8_t brightness = 100;
    bool initialized = false;
};

#endif // DEVICE_CONTROL_H
