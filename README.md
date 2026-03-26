# Simple Smart Home System

极简智能家居控制系统 - 一个真正简单、可量产的解决方案

## 系统架构

```
┌─────────────────┐     WiFi      ┌──────────────────┐
│   ESP32 设备    │ ◄──────────► │  OpenClaw Gateway │
│  (灯具/插座等)   │   局域网API   │    管理端界面     │
└─────────────────┘              └──────────────────┘
                                        │
                                        ▼
                                 ┌──────────────────┐
                                 │   手机配网APP    │
                                 │  (首次设置用)    │
                                 └──────────────────┘
```

## 核心特性

1. **设备端极简**
   - ESP32 主控，成本低（< ¥20）
   - 提供 RESTful API
   - 支持 UDP 自动发现

2. **Gateway 集成**
   - 自动发现局域网设备
   - Web 管理界面
   - OpenClaw Skill 控制

3. **手机配网**
   - 首次连接设备 WiFi
   - 配置家庭 WiFi + 本地 IP
   - 无需云服务

## 目录结构

```
smart-home-simple/
├── firmware/           # ESP32 固件代码
│   ├── src/
│   │   ├── main.cpp           # 主程序
│   │   ├── api_server.cpp     # HTTP API 服务
│   │   ├── wifi_config.cpp    # 配网模块
│   │   ├── device_control.cpp # 设备控制
│   │   └── udp_discovery.cpp  # UDP 设备发现
│   ├── platformio.ini
│   └── README.md
│
├── hardware/           # 硬件设计文档
│   ├── schematic.pdf          # 电路原理图
│   ├── pcb/                   # PCB 设计文件
│   ├── bom.csv                # 物料清单
│   └── enclosure/             # 外壳设计
│
├── gateway-skill/      # OpenClaw Skill
│   ├── SKILL.md
│   └── scripts/
│       ├── discover.py        # 设备发现
│       ├── control.py         # 设备控制
│       └── web/               # 管理界面
│
├── mobile-config/      # 手机配网界面
│   └── index.html
│
└── docs/               # 文档
    ├── API.md                 # 设备 API 文档
    ├── PRODUCTION.md          # 生产指南
    └── INTEGRATION.md         # 集成指南
```

## 快速开始

### 1. 烧录固件

```bash
cd firmware
pio run -t upload
```

### 2. 首次配网

1. 手机连接 WiFi "SmartHome-XXXXXX"
2. 访问 192.168.4.1
3. 输入家庭 WiFi 信息
4. 设置设备本地 IP

### 3. Gateway 管理

设备会自动出现在 Gateway 管理界面。

## API 文档

详见 [docs/API.md](docs/API.md)

## 生产制造

详见 [docs/PRODUCTION.md](docs/PRODUCTION.md)

## License

MIT
