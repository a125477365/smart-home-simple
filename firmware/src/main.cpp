/**
 * Simple Smart Home Device - ESP32 固件
 * 极简智能家居设备主程序（模块化版本）
 * 
 * 模块结构：
 * - config.h: 配置常量
 * - config_store: 配置存储（Preferences 封装）
 * - device_control: 继电器/调光控制
 * - wifi_manager: WiFi/AP 模式管理
 * - api_server: RESTful API 服务
 * - udp_discovery: UDP 设备发现
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include "config.h"
#include "config_store.h"
#include "device_control.h"
#include "wifi_manager.h"
#include "api_server.h"
#include "udp_discovery.h"

// ========== 按钮处理 ==========
void handleResetButton() {
    static unsigned long pressStart = 0;
    
    if (digitalRead(BUTTON_PIN) == LOW) {
        if (pressStart == 0) {
            pressStart = millis();
        } else if (millis() - pressStart > RESET_BUTTON_TIME) {
            Serial.println("Reset button pressed, clearing config...");
            ConfigStore::instance().clearNetworkConfig();
            delay(1000);
            ESP.restart();
        }
    } else {
        pressStart = 0;
    }
}

// ========== 初始化 ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== Smart Home Device v" + String(FIRMWARE_VERSION) + " ===");
    
    // 初始化 SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed, formatting...");
        SPIFFS.format();
        SPIFFS.begin();
    }
    Serial.printf("SPIFFS: %u bytes used\n", SPIFFS.usedBytes());
    
    // 初始化按钮
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // 初始化模块
    DeviceControl::instance().begin();
    WifiManager::instance().begin();
    
    // 启动服务
    ApiServer::instance().begin();
    UdpDiscovery::instance().begin();
    
    Serial.println("Device ready!");
}

// ========== 主循环 ==========
void loop() {
    // WiFi 状态维护
    WifiManager::instance().loop();
    
    // UDP 发现处理
    UdpDiscovery::instance().handle();
    
    // 重置按钮检测
    handleResetButton();
    
    delay(10);
}
