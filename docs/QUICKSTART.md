# 快速开始指南

## 一、准备工作

### 硬件

- ESP32 开发板（如 ESP32-DevKitC）
- 继电器模块（5V）
- 面包板和杜邦线
- USB 数据线

### 软件

- Python 3.8+
- PlatformIO（VSCode 插件或命令行）

## 二、烧录固件

### 1. 安装 PlatformIO

```bash
pip install platformio
```

### 2. 克隆项目

```bash
git clone https://github.com/your-username/smart-home-simple.git
cd smart-home-simple/firmware
```

### 3. 连接设备

将 ESP32 通过 USB 连接到电脑。

### 4. 编译并烧录

```bash
# 编译
pio run

# 烧录
pio run -t upload

# 查看日志
pio device monitor
```

## 三、首次配网

### 1. 设备进入配网模式

设备上电后会自动进入配网模式，LED 快闪。

### 2. 连接设备 WiFi

在手机 WiFi 设置中找到 "SmartHome-XXXXXX"，连接它（密码：12345678）。

### 3. 配网页面

浏览器会自动弹出配网页面，或手动访问 http://192.168.4.1

### 4. 配置 WiFi

1. 选择您的家庭 WiFi
2. 输入密码
3. （可选）设置静态 IP
4. 点击"连接"

设备将重启并连接到您的 WiFi。

## 四、安装 Gateway Skill

### 1. 安装依赖

```bash
cd ../gateway-skill
pip install -r requirements.txt
```

### 2. 启动 API 服务

```bash
python scripts/api_server.py
```

### 3. 访问 Web 界面

打开浏览器访问：http://localhost:5000/smarthome/

### 4. 发现设备

点击"发现新设备"按钮，系统会自动扫描局域网内的设备。

## 五、使用设备

### Web 界面

- 点击开关控制设备
- 拖动滑块调节亮度
- 点击"编辑名称"自定义设备名称

### 自然语言（如已集成 OpenClaw）

```
打开客厅的灯
把卧室灯调暗一点
关闭所有灯
```

### API 调用

```bash
# 获取设备列表
curl http://localhost:5000/api/smarthome/devices

# 控制设备
curl -X POST http://localhost:5000/api/smarthome/control/<device-id> \
  -H "Content-Type: application/json" \
  -d '{"state": true, "brightness": 80}'
```

## 六、硬件组装

如果需要制作实体设备：

1. 参考 `hardware/README.md` 中的电路图
2. 在立创商城采购元器件
3. 在嘉立创下单 PCB
4. 组装并测试

## 下一步

- 阅读 [API 文档](docs/API.md)
- 了解 [生产制造](docs/PRODUCTION.md)
- 配置 [Gateway 集成](docs/INTEGRATION.md)

## 问题排查

### 设备无法连接 WiFi

- 检查 WiFi 密码是否正确
- 确认 WiFi 是 2.4GHz（ESP32 不支持 5GHz）
- 尝试靠近路由器

### Gateway 无法发现设备

- 确保设备和 Gateway 在同一局域网
- 检查防火墙是否阻止 UDP 43210 端口
- 尝试手动添加设备（输入 IP）

### 设备控制无响应

- 检查设备 IP 是否变化（建议设置静态 IP）
- 重启设备
- 检查设备日志（串口监视器）
