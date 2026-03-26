# 设备 API 文档

## 概述

所有设备通过 HTTP RESTful API 提供服务，默认端口 80。

## 基础 URL

```
http://<device-ip>/api/
```

## 通用响应

所有响应均为 JSON 格式，包含以下字段：

```json
{
    "success": true,
    "data": {...},
    "error": null
}
```

---

## API 端点

### 1. 获取设备信息

**GET** `/api/info`

获取设备完整信息。

**响应示例：**

```json
{
    "id": "a1b2c3d4e5f6",
    "name": "客厅灯",
    "type": "light",
    "version": "1.0.0",
    "ip": "192.168.1.100",
    "state": true,
    "brightness": 80,
    "wifi_ssid": "MyWiFi",
    "rssi": -45
}
```

### 2. 控制设备

**POST** `/api/control`

控制设备状态、亮度、名称等。

**请求体：**

```json
{
    "state": true,
    "brightness": 50,
    "name": "客厅灯"
}
```

所有字段均为可选。

**响应示例：**

```json
{
    "success": true,
    "state": true,
    "brightness": 50
}
```

### 3. 获取当前状态

**GET** `/api/state`

仅获取设备状态，响应更轻量。

**响应示例：**

```json
{
    "state": true,
    "brightness": 80
}
```

---

## 配网 API（AP 模式）

### 4. 扫描 WiFi

**GET** `/api/scan`

扫描附近的 WiFi 网络。

**响应示例：**

```json
{
    "networks": [
        {"ssid": "WiFi1", "rssi": -45, "encrypted": true},
        {"ssid": "WiFi2", "rssi": -60, "encrypted": false}
    ]
}
```

### 5. 配置 WiFi

**POST** `/api/config`

配置设备连接的 WiFi 网络。

**请求体：**

```json
{
    "ssid": "MyWiFi",
    "password": "mypassword",
    "ip": "192.168.1.100",
    "gateway": "192.168.1.1",
    "subnet": "255.255.255.0"
}
```

| 字段 | 必填 | 说明 |
|------|------|------|
| ssid | 是 | WiFi 名称 |
| password | 是 | WiFi 密码 |
| ip | 否 | 静态 IP，不填则使用 DHCP |
| gateway | 否 | 网关地址 |
| subnet | 否 | 子网掩码，默认 255.255.255.0 |

**响应示例：**

```json
{
    "success": true,
    "message": "rebooting"
}
```

设备将重启并连接到指定 WiFi。

### 6. 重置设备

**POST** `/api/reset`

清除所有配置，恢复出厂设置。

**响应示例：**

```json
{
    "success": true,
    "message": "rebooting"
}
```

---

## UDP 设备发现

设备监听 UDP 端口 `43210`，响应发现请求。

### 发现请求

发送广播包：

```
目标地址: 255.255.255.255:43210
内容: SMART_HOME_DISCOVER
```

### 发现响应

设备响应 JSON 格式的设备信息：

```json
{
    "id": "a1b2c3d4e5f6",
    "name": "客厅灯",
    "type": "light",
    "ip": "192.168.1.100",
    "version": "1.0.0",
    "state": true
}
```

---

## 设备类型

| type | 说明 | 支持字段 |
|------|------|----------|
| light | 灯具 | state, brightness |
| switch | 插座/开关 | state |
| sensor | 传感器 | state, value |

---

## 错误码

| HTTP 状态码 | 说明 |
|-------------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 404 | 端点不存在 |
| 500 | 设备内部错误 |

---

## 示例代码

### Python

```python
import requests

# 控制设备
def control_device(ip, state=None, brightness=None):
    payload = {}
    if state is not None:
        payload['state'] = state
    if brightness is not None:
        payload['brightness'] = brightness
    
    response = requests.post(f'http://{ip}/api/control', json=payload)
    return response.json()

# 打开灯
result = control_device('192.168.1.100', state=True)
print(result)
```

### curl

```bash
# 获取设备信息
curl http://192.168.1.100/api/info

# 打开设备
curl -X POST http://192.168.1.100/api/control \
  -H "Content-Type: application/json" \
  -d '{"state": true}'

# 设置亮度
curl -X POST http://192.168.1.100/api/control \
  -H "Content-Type: application/json" \
  -d '{"brightness": 50}'
```
