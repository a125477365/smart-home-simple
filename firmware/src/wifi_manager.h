/**
 * WiFi 管理模块 - AP/STA 模式切换
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config_store.h"

enum class WifiMode {
    UNCONFIGURED,   // 未配置，需要配网
    CONNECTING,     // 正在连接
    CONNECTED,      // 已连接
    AP_MODE,        // AP 配网模式
    FAILED          // 连接失败
};

class WifiManager {
public:
    static WifiManager& instance();
    
    void begin();
    void loop();
    
    // 配网模式
    void startAPMode();
    void stopAPMode();
    bool isInAPMode() const { return currentMode == WifiMode::AP_MODE; }
    
    // 连接状态
    WifiMode getMode() const { return currentMode; }
    bool isConnected() const { return currentMode == WifiMode::CONNECTED; }
    String getLocalIP() const;
    String getAPName() const;
    
    // 配置
    void connectToWifi(const NetworkConfig& config);
    void disconnect();
    
    // 状态 LED
    void updateStatusLED();

private:
    WifiManager() = default;
    WifiMode currentMode = WifiMode::UNCONFIGURED;
    unsigned long lastActivity = 0;
    String apName;
    
    void setupStaticIP(const NetworkConfig& config);
};

#endif // WIFI_MANAGER_H
