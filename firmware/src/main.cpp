/**
 * Simple Smart Home Device - ESP32 固件
 * 极简智能家居设备主程序
 * 
 * 功能：
 * - WiFi 配网（AP 模式）
 * - RESTful API 服务
 * - UDP 设备发现
 * - 设备控制（继电器/调光）
 */

#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// ========== 配置区 ==========
#define AP_NAME_PREFIX "SmartHome"
#define AP_PASSWORD "12345678"
#define UDP_DISCOVERY_PORT 43210
#define HTTP_API_PORT 80

// 硬件引脚
#define RELAY_PIN 25      // 继电器控制引脚
#define LED_PIN 2         // 状态指示灯
#define BUTTON_PIN 0      // 配置按钮（用于重置）

// ========== 全局变量 ==========
AsyncWebServer server(HTTP_API_PORT);
WiFiUDP udp;

// 设备信息
struct DeviceInfo {
    String id;
    String name;
    String type;
    String version;
    String localIp;
    bool state;
    uint8_t brightness;  // 0-100
} device;

// 网络配置（存储在 EEPROM）
struct NetworkConfig {
    char ssid[64];
    char password[64];
    char localIp[16];
    char gateway[16];
    char subnet[16];
} netConfig;

// ========== 原型声明 ==========
void setupAPMode();
void setupAPIServer();
void handleUDPDiscovery();
void loadConfig();
void saveConfig();
void resetDevice();
void setRelay(bool state);
void setBrightness(uint8_t value);

// ========== 主程序 ==========

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== Smart Home Device Starting ===");
    
    // 初始化硬件
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // 生成设备 ID（基于 MAC 地址）
    uint64_t chipid = ESP.getEfuseMac();
    device.id = String((uint32_t)(chipid >> 16), HEX) + String((uint32_t)chipid, HEX);
    device.name = "Smart Device";
    device.type = "light";
    device.version = "1.0.0";
    device.state = false;
    device.brightness = 100;
    
    // 加载配置
    loadConfig();
    
    // 检查是否需要进入配网模式
    bool needConfig = (strlen(netConfig.ssid) == 0);
    
    // 检查重置按钮（长按 5 秒）
    unsigned long pressStart = 0;
    while (digitalRead(BUTTON_PIN) == LOW) {
        if (pressStart == 0) pressStart = millis();
        else if (millis() - pressStart > 5000) {
            Serial.println("Reset button pressed, clearing config...");
            resetDevice();
            needConfig = true;
            break;
        }
        delay(100);
    }
    
    if (needConfig) {
        // 进入配网模式
        setupAPMode();
    } else {
        // 连接 WiFi
        Serial.printf("Connecting to WiFi: %s\n", netConfig.ssid);
        WiFi.begin(netConfig.ssid, netConfig.password);
        
        // 设置静态 IP
        if (strlen(netConfig.localIp) > 0) {
            IPAddress ip, gw, sn;
            ip.fromString(netConfig.localIp);
            gw.fromString(netConfig.gateway);
            sn.fromString(netConfig.subnet);
            WiFi.config(ip, gw, sn);
        }
        
        // 等待连接
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            device.localIp = WiFi.localIP().toString();
            Serial.printf("\nConnected! IP: %s\n", device.localIp.c_str());
            digitalWrite(LED_PIN, HIGH);
        } else {
            Serial.println("\nWiFi connection failed, entering AP mode...");
            setupAPMode();
        }
    }
    
    // 启动 API 服务器
    setupAPIServer();
    
    // 启动 UDP 发现
    udp.begin(UDP_DISCOVERY_PORT);
    
    Serial.println("Device ready!");
}

void loop() {
    // 处理 UDP 发现
    handleUDPDiscovery();
    
    // 状态指示
    static unsigned long lastBlink = 0;
    if (WiFi.status() == WL_CONNECTED) {
        if (millis() - lastBlink > 2000) {
            lastBlink = millis();
            digitalWrite(LED_PIN, HIGH);
        }
    }
    
    delay(10);
}

// ========== 配网模式 ==========
void setupAPMode() {
    String apName = String(AP_NAME_PREFIX) + "-" + device.id.substring(0, 6);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), AP_PASSWORD);
    
    device.localIp = "192.168.4.1";
    Serial.printf("AP Mode: %s, IP: %s\n", apName.c_str(), device.localIp.c_str());
    
    // 快闪指示
    digitalWrite(LED_PIN, HIGH);
}

// ========== API 服务器 ==========
void setupAPIServer() {
    // CORS 支持
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // OPTIONS 预检请求
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });
    
    // ========== 设备信息 API ==========
    // GET /api/info - 获取设备信息
    server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["id"] = device.id;
        doc["name"] = device.name;
        doc["type"] = device.type;
        doc["version"] = device.version;
        doc["ip"] = device.localIp;
        doc["state"] = device.state;
        doc["brightness"] = device.brightness;
        doc["wifi_ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // ========== 设备控制 API ==========
    // POST /api/control - 控制设备
    server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *request) {
        // 解析 JSON body
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"error\":\"missing body\"}");
            return;
        }
        
        String body = request->getParam("plain", true)->value();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        
        // 处理控制命令
        if (doc.containsKey("state")) {
            device.state = doc["state"];
            setRelay(device.state);
        }
        
        if (doc.containsKey("brightness")) {
            device.brightness = constrain((int)doc["brightness"], 0, 100);
            setBrightness(device.brightness);
        }
        
        if (doc.containsKey("name")) {
            device.name = doc["name"].as<String>();
        }
        
        // 返回更新后的状态
        JsonDocument response;
        response["success"] = true;
        response["state"] = device.state;
        response["brightness"] = device.brightness;
        
        String respStr;
        serializeJson(response, respStr);
        request->send(200, "application/json", respStr);
    });
    
    // GET /api/state - 获取当前状态
    server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["state"] = device.state;
        doc["brightness"] = device.brightness;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // ========== WiFi 配网 API ==========
    // GET /api/scan - 扫描 WiFi
    server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanComplete();
        if (n == -2) {
            WiFi.scanNetworks(true);
            request->send(202, "application/json", "{\"status\":\"scanning\"}");
        } else {
            JsonDocument doc;
            JsonArray networks = doc["networks"].to<JsonArray>();
            for (int i = 0; i < n; i++) {
                JsonObject net = networks.add<JsonObject>();
                net["ssid"] = WiFi.SSID(i);
                net["rssi"] = WiFi.RSSI(i);
                net["encrypted"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
            }
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
            WiFi.scanDelete();
        }
    });
    
    // POST /api/config - 配置 WiFi
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("plain", true)) {
            request->send(400, "application/json", "{\"error\":\"missing body\"}");
            return;
        }
        
        String body = request->getParam("plain", true)->value();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            request->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        
        // 保存配置
        strlcpy(netConfig.ssid, doc["ssid"] | "", sizeof(netConfig.ssid));
        strlcpy(netConfig.password, doc["password"] | "", sizeof(netConfig.password));
        strlcpy(netConfig.localIp, doc["ip"] | "", sizeof(netConfig.localIp));
        strlcpy(netConfig.gateway, doc["gateway"] | "", sizeof(netConfig.gateway));
        strlcpy(netConfig.subnet, doc["subnet"] | "255.255.255.0", sizeof(netConfig.subnet));
        
        saveConfig();
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"rebooting\"}");
        
        // 延迟重启
        delay(1000);
        ESP.restart();
    });
    
    // POST /api/reset - 重置设备
    server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        resetDevice();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"rebooting\"}");
        delay(1000);
        ESP.restart();
    });
    
    server.begin();
    Serial.println("API server started on port " + String(HTTP_API_PORT));
}

// ========== UDP 设备发现 ==========
void handleUDPDiscovery() {
    int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;
    
    char buffer[256];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = 0;
    
    // 检查发现请求
    if (strcmp(buffer, "SMART_HOME_DISCOVER") == 0) {
        Serial.println("Discovery request received");
        
        // 响应设备信息
        JsonDocument doc;
        doc["id"] = device.id;
        doc["name"] = device.name;
        doc["type"] = device.type;
        doc["ip"] = device.localIp;
        doc["version"] = device.version;
        doc["state"] = device.state;
        
        String response;
        serializeJson(doc, response);
        
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(response);
        udp.endPacket();
    }
}

// ========== 设备控制 ==========
void setRelay(bool state) {
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
    Serial.printf("Relay set to: %s\n", state ? "ON" : "OFF");
}

void setBrightness(uint8_t value) {
    // 如果支持调光，这里使用 PWM
    // 目前简单实现：brightness > 50 时开灯
    Serial.printf("Brightness set to: %d\n", value);
}

// ========== 配置存储 ==========
void loadConfig() {
    // 从 EEPROM 加载配置
    // 简化实现：使用 Preferences
    Preferences prefs;
    prefs.begin("smarthome", true);
    
    prefs.getString("ssid", netConfig.ssid, sizeof(netConfig.ssid));
    prefs.getString("password", netConfig.password, sizeof(netConfig.password));
    prefs.getString("ip", netConfig.localIp, sizeof(netConfig.localIp));
    prefs.getString("gateway", netConfig.gateway, sizeof(netConfig.gateway));
    prefs.getString("subnet", netConfig.subnet, sizeof(netConfig.subnet));
    
    device.name = prefs.getString("name", device.name);
    prefs.end();
    
    Serial.println("Config loaded");
}

void saveConfig() {
    Preferences prefs;
    prefs.begin("smarthome", false);
    
    prefs.putString("ssid", netConfig.ssid);
    prefs.putString("password", netConfig.password);
    prefs.putString("ip", netConfig.localIp);
    prefs.putString("gateway", netConfig.gateway);
    prefs.putString("subnet", netConfig.subnet);
    prefs.putString("name", device.name);
    
    prefs.end();
    Serial.println("Config saved");
}

void resetDevice() {
    Preferences prefs;
    prefs.begin("smarthome", false);
    prefs.clear();
    prefs.end();
    Serial.println("Device reset");
}
