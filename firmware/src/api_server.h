/**
 * API 服务器模块 - RESTful 接口
 */
#ifndef API_SERVER_H
#define API_SERVER_H

#include <Arduino.h>

class ApiServer {
public:
    static ApiServer& instance();
    
    void begin();
    void stop();
    
    // 设备信息
    String getDeviceInfo();
    String getStateInfo();

private:
    ApiServer() = default;
    bool running = false;
    
    void setupRoutes();
    void setupCORS();
};

#endif // API_SERVER_H
