/**
 * UDP 设备发现模块实现
 */
#include "udp_discovery.h"
#include "config.h"
#include "config_store.h"
#include "device_control.h"
#include "wifi_manager.h"

#include <WiFi.h>
#include <ArduinoJson.h>

static WiFiUDP udp;

UdpDiscovery& UdpDiscovery::instance() {
    static UdpDiscovery _instance;
    return _instance;
}

void UdpDiscovery::begin() {
    if (started) return;
    
    if (udp.begin(UDP_DISCOVERY_PORT)) {
        started = true;
        Serial.println("UDP discovery started on port " + String(UDP_DISCOVERY_PORT));
    } else {
        Serial.println("UDP discovery start failed!");
    }
}

void UdpDiscovery::handle() {
    if (!started) return;
    
    int packetSize = udp.parsePacket();
    if (packetSize <= 0) return;
    
    char buffer[256];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    buffer[len] = 0;
    
    // 检查发现请求
    if (strcmp(buffer, "SMART_HOME_DISCOVER") == 0) {
        Serial.printf("Discovery request from %s:%d\n", 
                      udp.remoteIP().toString().c_str(), udp.remotePort());
        sendResponse();
    }
}

void UdpDiscovery::sendResponse() {
    JsonDocument doc;
    doc["id"] = ConfigStore::instance().getDeviceId();
    doc["name"] = ConfigStore::instance().loadDeviceName();
    doc["type"] = "light";
    doc["ip"] = WifiManager::instance().getLocalIP();
    doc["version"] = FIRMWARE_VERSION;
    doc["state"] = DeviceControl::instance().getRelayState();
    
    String response;
    serializeJson(doc, response);
    
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print(response);
    udp.endPacket();
    
    Serial.println("Discovery response sent");
}
