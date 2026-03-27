# Simple Smart Home System

极简智能家居控制系统 - 一个真正简单、可量产的解决方案

## 系统架构

```
┌─────────────────┐      WiFi      ┌──────────────────┐
│   ESP32 设备    │◄──────────────►│  OpenClaw Gateway │
│  (灯具/插座等)  │    局域网API    │   (原生集成)     │
└─────────────────┘                 └──────────────────┘
        │                                    │
        ▼                                    ▼
┌──────────────────┐              ┌──────────────────┐
│  配网 Web 页面   │              │   控制面板界面   │
│ (内置 SPIFFS)    │              │ (/smarthome)     │
└──────────────────┘              └──────────────────┘
```

## 核心特性

### 1. 设备端极简
- ESP32 主控，成本低（< ¥20）
- 模块化固件架构，易于维护
- 提供 RESTful API
- 支持 UDP 自动发现
- **配网页面内置**（SPIFFS 文件系统）

### 2. Gateway 原生集成
- TypeScript 插件，深度集成 OpenClaw
- 自动发现局域网设备
- 原生 Web 管理界面（/smarthome）
- 自然语言控制（通过 Agent 工具）

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
├── gateway-plugin/        # OpenClaw 原生插件
│   ├── src/
│   │   ├── index.ts       # 插件入口
│   │   ├── device-manager.ts
│   │   ├── web-routes.ts
│   │   └── web/panel.html # 控制面板
│   ├── package.json
│   └── openclaw.plugin.json
│
├── hardware/              # 硬件设计文档
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
5. 点击"连接"，设备重启

### 3. 安装 Gateway 插件

```bash
cd gateway-plugin
pnpm install
pnpm build

# 链接到 OpenClaw
ln -s $(pwd) ~/.openclaw/plugins/smart-home
```

### 4. 访问控制面板

启动 Gateway 后访问：http://localhost:port/smarthome

## API 文档

详见 [docs/API.md](docs/API.md)

## 生产制造

详见 [docs/PRODUCTION.md](docs/PRODUCTION.md)

## 自然语言控制

集成后可通过 OpenClaw Agent 控制：

```
发现智能家居设备
打开客厅的灯
把卧室灯亮度调到 50%
关闭所有灯
```

## 版本历史

### v1.2.0 (当前)
- 重构为 OpenClaw 原生插件
- 删除独立 Python Gateway
- 添加 TypeScript 设备管理器
- 原生 Web 控制面板

### v1.1.0
- 模块化重构固件代码
- Gateway 改用 Quart 异步框架
- 添加状态掉电保存

### v1.0.0
- 初始版本

## License

MIT
