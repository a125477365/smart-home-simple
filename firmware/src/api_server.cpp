/**
 * API 服务器模块实现
 */
#include "api_server.h"
#include "config.h"
#include "config_store.h"
#include "device_control.h"
#include "wifi_manager.h"

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

static AsyncWebServer server(HTTP_API_PORT);

ApiServer& ApiServer::instance() {
    static ApiServer _instance;
    return _instance;
}

void ApiServer::begin() {
    if (running) return;
    
    setupCORS();
    setupRoutes();
    
    server.begin();
    running = true;
    Serial.println("API server started on port " + String(HTTP_API_PORT));
}

void ApiServer::stop() {
    if (!running) return;
    server.end();
    running = false;
}

void ApiServer::setupCORS() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void ApiServer::setupRoutes() {
    // OPTIONS 预检
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404, "application/json", "{\"error\":\"not found\"}");
        }
    });
    
    // ========== 静态文件 ==========
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });
    server.serveStatic("/", SPIFFS, "/");
    
    // ========== 设备信息 API ==========
    server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["id"] = ConfigStore::instance().getDeviceId();
        doc["name"] = ConfigStore::instance().loadDeviceName();
        doc["type"] = "light";
        doc["version"] = FIRMWARE_VERSION;
        doc["ip"] = WifiManager::instance().getLocalIP();
        doc["state"] = DeviceControl::instance().getRelayState();
        doc["brightness"] = DeviceControl::instance().getBrightness();
        doc["wifi_ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // ========== 设备控制 API ==========
    server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *request) {
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
            DeviceControl::instance().setRelay(doc["state"]);
        }
        if (doc.containsKey("brightness")) {
            DeviceControl::instance().setBrightness(doc["brightness"]);
        }
        if (doc.containsKey("name")) {
            ConfigStore::instance().saveDeviceName(doc["name"].as<String>());
        }
        
        // 保存状态
        DeviceControl::instance().saveState();
        
        // 返回更新后的状态
        JsonDocument response;
        response["success"] = true;
        response["state"] = DeviceControl::instance().getRelayState();
        response["brightness"] = DeviceControl::instance().getBrightness();
        
        String respStr;
        serializeJson(response, respStr);
        request->send(200, "application/json", respStr);
    });
    
    // GET /api/state
    server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["state"] = DeviceControl::instance().getRelayState();
        doc["brightness"] = DeviceControl::instance().getBrightness();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    // ========== WiFi 配网 API ==========
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
        NetworkConfig config;
        config.clear();
        strlcpy(config.ssid, doc["ssid"] | "", sizeof(config.ssid));
        strlcpy(config.password, doc["password"] | "", sizeof(config.password));
        strlcpy(config.localIp, doc["ip"] | "", sizeof(config.localIp));
        strlcpy(config.gateway, doc["gateway"] | "", sizeof(config.gateway));
        strlcpy(config.subnet, doc["subnet"] | "255.255.255.0", sizeof(config.subnet));
        
        ConfigStore::instance().saveNetworkConfig(config);
        
        // 保存设备名称
        if (doc.containsKey("name")) {
            ConfigStore::instance().saveDeviceName(doc["name"].as<String>());
        }
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"rebooting\"}");
        
        // 延迟重启
        delay(1000);
        ESP.restart();
    });
    
    server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        ConfigStore::instance().clearNetworkConfig();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"rebooting\"}");
        delay(1000);
        ESP.restart();
    });
}

String ApiServer::getDeviceInfo() {
    JsonDocument doc;
    doc["id"] = ConfigStore::instance().getDeviceId();
    doc["name"] = ConfigStore::instance().loadDeviceName();
    doc["type"] = "light";
    doc["ip"] = WifiManager::instance().getLocalIP();
    doc["version"] = FIRMWARE_VERSION;
    doc["state"] = DeviceControl::instance().getRelayState();
    
    String response;
    serializeJson(doc, response);
    return response;
}

String ApiServer::getStateInfo() {
    JsonDocument doc;
    doc["state"] = DeviceControl::instance().getRelayState();
    doc["brightness"] = DeviceControl::instance().getBrightness();
    
    String response;
    serializeJson(doc, response);
    return response;
}
