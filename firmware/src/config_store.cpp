/**
 * 配置存储模块实现
 */
#include "config_store.h"
#include "esp_system.h"

ConfigStore& ConfigStore::instance() {
    static ConfigStore _instance;
    return _instance;
}

void ConfigStore::begin() {
    if (!initialized) {
        prefs.begin("smarthome", false);
        initialized = true;
    }
}

void ConfigStore::end() {
    if (initialized) {
        prefs.end();
        initialized = false;
    }
}

String ConfigStore::getDeviceId() {
    if (deviceId.isEmpty()) {
        uint64_t chipid = ESP.getEfuseMac();
        deviceId = String((uint32_t)(chipid >> 16), HEX) + String((uint32_t)chipid, HEX);
    }
    return deviceId;
}

NetworkConfig ConfigStore::loadNetworkConfig() {
    NetworkConfig config;
    config.clear();
    
    prefs.begin("smarthome", true);
    prefs.getString("ssid", config.ssid, sizeof(config.ssid));
    prefs.getString("password", config.password, sizeof(config.password));
    prefs.getString("ip", config.localIp, sizeof(config.localIp));
    prefs.getString("gateway", config.gateway, sizeof(config.gateway));
    prefs.getString("subnet", config.subnet, sizeof(config.subnet));
    prefs.end();
    
    return config;
}

void ConfigStore::saveNetworkConfig(const NetworkConfig& config) {
    prefs.begin("smarthome", false);
    prefs.putString("ssid", config.ssid);
    prefs.putString("password", config.password);
    prefs.putString("ip", config.localIp);
    prefs.putString("gateway", config.gateway);
    prefs.putString("subnet", config.subnet);
    prefs.end();
}

void ConfigStore::clearNetworkConfig() {
    prefs.begin("smarthome", false);
    prefs.clear();
    prefs.end();
}

String ConfigStore::loadDeviceName() {
    prefs.begin("smarthome", true);
    String name = prefs.getString("name", "Smart Device");
    prefs.end();
    return name;
}

void ConfigStore::saveDeviceName(const String& name) {
    prefs.begin("smarthome", false);
    prefs.putString("name", name);
    prefs.end();
}

bool ConfigStore::loadRelayState() {
    prefs.begin("smarthome", true);
    bool state = prefs.getBool("relay", false);
    prefs.end();
    return state;
}

void ConfigStore::saveRelayState(bool state) {
    prefs.begin("smarthome", false);
    prefs.putBool("relay", state);
    prefs.end();
}

uint8_t ConfigStore::loadBrightness() {
    prefs.begin("smarthome", true);
    uint8_t brightness = prefs.getUChar("brightness", 100);
    prefs.end();
    return brightness;
}

void ConfigStore::saveBrightness(uint8_t brightness) {
    prefs.begin("smarthome", false);
    prefs.putUChar("brightness", brightness);
    prefs.end();
}
