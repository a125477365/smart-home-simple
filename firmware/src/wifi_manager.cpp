/**
 * WiFi 管理模块实现
 */
#include "wifi_manager.h"
#include "config.h"
#include "config_store.h"

WifiManager& WifiManager::instance() {
    static WifiManager _instance;
    return _instance;
}

void WifiManager::begin() {
    // 检查是否已配置
    NetworkConfig config = ConfigStore::instance().loadNetworkConfig();
    
    if (!config.isConfigured()) {
        Serial.println("No WiFi config, starting AP mode");
        startAPMode();
        return;
    }
    
    // 尝试连接
    connectToWifi(config);
}

void WifiManager::loop() {
    // 检查连接状态
    if (currentMode == WifiMode::CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, reconnecting...");
            currentMode = WifiMode::CONNECTING;
            WiFi.reconnect();
        }
    }
    
    // AP 模式超时检查
    if (currentMode == WifiMode::AP_MODE) {
        if (millis() - lastActivity > AP_MODE_TIMEOUT) {
            Serial.println("AP mode timeout, restarting...");
            ESP.restart();
        }
    }
    
    updateStatusLED();
}

void WifiManager::startAPMode() {
    // 生成 AP 名称
    apName = String(AP_NAME_PREFIX) + "-" + ConfigStore::instance().getDeviceId().substring(0, 6);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), AP_PASSWORD);
    
    currentMode = WifiMode::AP_MODE;
    lastActivity = millis();
    
    Serial.printf("AP Mode started: %s, IP: %s\n", apName.c_str(), 
                  WiFi.softAPIP().toString().c_str());
}

void WifiManager::stopAPMode() {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}

String WifiManager::getLocalIP() const {
    switch (currentMode) {
        case WifiMode::AP_MODE:
            return WiFi.softAPIP().toString();
        case WifiMode::CONNECTED:
            return WiFi.localIP().toString();
        default:
            return "0.0.0.0";
    }
}

String WifiManager::getAPName() const {
    return apName;
}

void WifiManager::connectToWifi(const NetworkConfig& config) {
    currentMode = WifiMode::CONNECTING;
    
    // 设置静态 IP（如果配置了）
    setupStaticIP(config);
    
    Serial.printf("Connecting to WiFi: %s\n", config.ssid);
    WiFi.begin(config.ssid, config.password);
    
    // 等待连接
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_TIMEOUT / 500) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        currentMode = WifiMode::CONNECTED;
        Serial.printf("\nConnected! IP: %s, RSSI: %d\n", 
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        digitalWrite(LED_PIN, HIGH);
    } else {
        currentMode = WifiMode::FAILED;
        Serial.println("\nWiFi connection failed, starting AP mode...");
        startAPMode();
    }
}

void WifiManager::setupStaticIP(const NetworkConfig& config) {
    if (strlen(config.localIp) > 0) {
        IPAddress ip, gw, sn;
        ip.fromString(config.localIp);
        gw.fromString(config.gateway);
        sn.fromString(strlen(config.subnet) > 0 ? config.subnet : "255.255.255.0");
        WiFi.config(ip, gw, sn);
        Serial.printf("Static IP configured: %s\n", config.localIp);
    }
}

void WifiManager::disconnect() {
    WiFi.disconnect(true);
    currentMode = WifiMode::UNCONFIGURED;
}

void WifiManager::updateStatusLED() {
    static unsigned long lastBlink = 0;
    
    switch (currentMode) {
        case WifiMode::AP_MODE:
            // AP 模式：快闪
            if (millis() - lastBlink > 200) {
                lastBlink = millis();
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            }
            break;
        case WifiMode::CONNECTING:
            // 连接中：中速闪
            if (millis() - lastBlink > 500) {
                lastBlink = millis();
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            }
            break;
        case WifiMode::CONNECTED:
            // 已连接：常亮
            digitalWrite(LED_PIN, HIGH);
            break;
        case WifiMode::FAILED:
            // 失败：慢闪
            if (millis() - lastBlink > 1000) {
                lastBlink = millis();
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            }
            break;
        default:
            break;
    }
}
