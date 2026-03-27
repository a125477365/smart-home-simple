/**
 * UDP 设备发现模块
 */
#ifndef UDP_DISCOVERY_H
#define UDP_DISCOVERY_H

#include <Arduino.h>

class UdpDiscovery {
public:
    static UdpDiscovery& instance();
    
    void begin();
    void handle();
    
private:
    UdpDiscovery() = default;
    bool started = false;
    
    void sendResponse();
};

#endif // UDP_DISCOVERY_H
