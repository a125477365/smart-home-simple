/**
 * 配置存储模块 - 封装 Preferences
 */
#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>
#include <Preferences.h>

struct NetworkConfig {
    char ssid[64];
    char password[64];
    char localIp[16];
    char gateway[16];
    char subnet[16];
    
    bool isConfigured() const {
        return strlen(ssid) > 0;
    }
    
    void clear() {
        memset(this, 0, sizeof(NetworkConfig));
    }
};

class ConfigStore {
public:
    static ConfigStore& instance();
    
    void begin();
    void end();
    
    // 网络配置
    NetworkConfig loadNetworkConfig();
    void saveNetworkConfig(const NetworkConfig& config);
    void clearNetworkConfig();
    
    // 设备配置
    String loadDeviceName();
    void saveDeviceName(const String& name);
    bool loadRelayState();
    void saveRelayState(bool state);
    uint8_t loadBrightness();
    void saveBrightness(uint8_t brightness);
    
    // 设备 ID（基于 MAC）
    String getDeviceId();
    
private:
    ConfigStore() = default;
    Preferences prefs;
    String deviceId;
    bool initialized = false;
};

#endif // CONFIG_STORE_H
