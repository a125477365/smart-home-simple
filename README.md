# Simple Smart Home System

极简智能家居控制系统 - 一个真正简单、可量产的解决方案

## 系统架构

```
┌─────────────────┐      WiFi      ┌──────────────────┐
│   ESP32 设备    │◄──────────────►│  OpenClaw Gateway │
│  (灯具/插座等)  │    局域网API    │    管理端界面     │
└─────────────────┘                 └──────────────────┘
        │
        ▼
┌──────────────────┐
│  配网 Web 页面   │
│ (内置 SPIFFS)    │
└──────────────────┘
```

## 核心特性

### 1. 设备端极简
- ESP32 主控，成本低（< ¥20）
- 模块化固件架构，易于维护
- 提供 RESTful API
- 支持 UDP 自动发现
- **配网页面内置**（SPIFFS 文件系统）

### 2. Gateway 集成
- 异步 API 服务器（Quart 框架）
- 自动发现局域网设备
- Web 管理界面
- OpenClaw Skill 控制

### 3. 手机配网
- 首次连接设备 WiFi（SmartHome-XXXXXX）
- 自动弹出配网页面
- 配置家庭 WiFi + 静态 IP
- 无需云服务

## 目录结构

```
smart-home-simple/
├── firmware/              # ESP32 固件代码（模块化）
│   ├── src/
│   │   ├── main.cpp       # 主入口
│   │   ├── config.h       # 配置常量
│   │   ├── config_store   # 配置存储模块
│   │   ├── device_control # 设备控制模块
│   │   ├── wifi_manager   # WiFi 管理模块
│   │   ├── api_server     # API 服务模块
│   │   └── udp_discovery  # UDP 发现模块
│   ├── data/
│   │   └── index.html     # 配网页面
│   └── platformio.ini
│
├── gateway-skill/         # OpenClaw Skill
│   ├── SKILL.md
│   ├── requirements.txt
│   └── scripts/
│       ├── api_server.py  # 异步 API 服务器
│       └── discover.py    # 设备发现脚本
│
├── hardware/              # 硬件设计文档
│   ├── README.md
│   └── bom.csv
│
└── docs/                  # 文档
    ├── API.md
    ├── PRODUCTION.md
    └── INTEGRATION.md
```

## 快速开始

### 1. 编译固件

```bash
cd firmware
pio run -t upload      # 烧录固件
pio run -t uploadfs    # 烧录文件系统（配网页面）
```

### 2. 首次配网

1. 设备上电后进入配网模式（LED 快闪）
2. 手机连接 WiFi "SmartHome-XXXXXX"
3. 浏览器自动弹出配网页面（或访问 192.168.4.1）
4. 选择家庭 WiFi 并输入密码
5. 可选：设置静态 IP
6. 点击"连接"，设备重启

### 3. Gateway 管理

设备会自动出现在 Gateway 管理界面。

## API 文档

详见 [docs/API.md](docs/API.md)

## 生产制造

详见 [docs/PRODUCTION.md](docs/PRODUCTION.md)

## 版本历史

### v1.1.0 (当前)
- 模块化重构固件代码
- Gateway 改用 Quart 异步框架
- 添加状态掉电保存
- 优化配网页面 UI
- AP 模式超时保护

### v1.0.0
- 初始版本
- 基础配网功能
- RESTful API
- UDP 设备发现

## License

MIT
