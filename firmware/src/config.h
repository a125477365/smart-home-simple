/**
 * 配置常量定义
 */
#ifndef CONFIG_H
#define CONFIG_H

// ========== 网络配置 ==========
#define AP_NAME_PREFIX "SmartHome"
#define AP_PASSWORD "12345678"
#define UDP_DISCOVERY_PORT 43210
#define HTTP_API_PORT 80

// ========== 硬件引脚 ==========
#define RELAY_PIN 25    // 继电器控制
#define LED_PIN 2       // 状态指示灯
#define BUTTON_PIN 0    // 配置按钮

// ========== 行为参数 ==========
#define WIFI_CONNECT_TIMEOUT 15000   // WiFi 连接超时（毫秒）
#define RESET_BUTTON_TIME 5000        // 重置按钮长按时间（毫秒）
#define AP_MODE_TIMEOUT 300000        // AP 模式超时（5分钟无操作重启）

// ========== 版本信息 ==========
#define FIRMWARE_VERSION "1.1.0"

#endif // CONFIG_H
