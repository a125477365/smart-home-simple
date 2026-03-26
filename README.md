# Simple Smart Home System
极简智能家居控制系统 - 一个真正简单、可量产的解决方案

## 系统架构

```
┌─────────────────┐         WiFi          ┌──────────────────┐
│   ESP32 设备    │ ◄──────────────────► │  OpenClaw Gateway │
│  (灯具/插座等)   │      局域网API        │    管理端界面     │
└─────────────────┘                       └──────────────────┘
        │
        ▼
┌──────────────────┐
│   配网 Web 页面   │
│  (内置 SPIFFS)   │
└──────────────────┘
```

## 核心特性

1. **设备端极简**
   - ESP32 主控，成本低（< ¥20）
   - 提供 RESTful API
   - 支持 UDP 自动发现
   - **配网页面内置**（SPIFFS 文件系统）

2. **Gateway 集成**
   - 自动发现局域网设备
   - Web 管理界面
   - OpenClaw Skill 控制

3. **手机配网**
   - 首次连接设备 WiFi（SmartHome-XXXXXX）
   - 自动弹出配网页面（captive portal）
   - 配置家庭 WiFi + 静态 IP
   - 无需云服务

## 目录结构

```
smart-home-simple/
├── firmware/           # ESP32 固件代码
│   ├── src/
│   │   └── main.cpp    # 主程序（含 API + 配网）
│   ├── data/           # SPIFFS 文件系统
│   │   └── index.html  # 配网页面
│   └── platformio.ini
│
├── hardware/           # 硬件设计文档
│   ├── schematic.pdf   # 电路原理图
│   ├── pcb/            # PCB 设计文件
│   ├── bom.csv         # 物料清单
│   └── enclosure/      # 外壳设计
│
├── gateway-skill/      # OpenClaw Skill
│   ├── SKILL.md
│   └── scripts/
│       ├── discover.py # 设备发现
│       ├── control.py  # 设备控制
│       └── web/        # 管理界面
│
└── docs/               # 文档
    ├── API.md          # 设备 API 文档
    ├── PRODUCTION.md   # 生产指南
    └── INTEGRATION.md  # 集成指南
```

## 快速开始

### 1. 编译固件

```bash
cd firmware
pio run -t upload       # 烧录固件
pio run -t uploadfs     # 烧录文件系统（配网页面）
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

## 技术细节

### 配网页面原理

- 配网页面存储在 SPIFFS 文件系统中
- 编译时通过 `pio run -t uploadfs` 上传到设备
- 设备启动时从 SPIFFS 读取 HTML 提供配网服务
- 无需外部服务器，完全自包含

### 为什么用 SPIFFS？

1. **自包含**：固件 + 配网页面一体化
2. **可更新**：修改 HTML 后单独上传文件系统
3. **低成本**：不需要外部存储芯片
4. **量产友好**：一次烧录，开箱即用

## License

MIT
