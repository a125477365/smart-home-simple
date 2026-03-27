# OpenClaw Gateway 集成指南

## 概述

本指南说明如何将智能家居系统原生集成到 OpenClaw Gateway。

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│                    OpenClaw Gateway                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  Web UI     │  │  Agent API  │  │  Plugin System      │ │
│  │ /smarthome  │  │  Tool Calls │  │  gateway-plugin     │ │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │                │                     │            │
│         └────────────────┴─────────────────────┘            │
│                           │                                  │
└───────────────────────────┼──────────────────────────────────┘
                            │ HTTP API
                            ▼
              ┌────────────────────────┐
              │  局域网智能设备         │
              │  (ESP32 + REST API)    │
              └────────────────────────┘
```

## 安装插件

### 方式一：开发模式

```bash
cd gateway-plugin
pnpm install
pnpm build

# 创建符号链接
mkdir -p ~/.openclaw/plugins
ln -s $(pwd) ~/.openclaw/plugins/smart-home
```

### 方式二：从 ClawHub 安装（发布后）

```bash
openclaw plugins install @openclaw/plugin-smart-home
```

## 配置

编辑 `~/.openclaw/config/openclaw.json`：

```json
{
  "plugins": {
    "smart-home": {
      "apiPort": 43210,
      "autoDiscover": true,
      "discoverInterval": 60000
    }
  }
}
```

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| apiPort | number | 43210 | UDP 发现端口 |
| autoDiscover | boolean | true | 启动时自动发现 |
| discoverInterval | number | 60000 | 发现间隔（毫秒） |

## 使用方式

### 1. Web 控制面板

访问 Gateway 的 `/smarthome` 路径：

```
http://localhost:3000/smarthome
```

功能：
- 设备列表展示
- 开关控制
- 亮度调节
- 设备发现
- 重命名/移除

### 2. 自然语言控制

通过 Agent 对话控制：

```
用户: 发现智能家居设备
助手: [调用 smart_home_discover 工具]

用户: 打开客厅的灯
助手: [调用 smart_home_control 工具]

用户: 关闭所有灯
助手: [调用 smart_home_all_off 工具]
```

### 3. HTTP API

直接调用 Gateway API：

```bash
# 获取设备列表
curl http://localhost:3000/api/smarthome/devices

# 控制设备
curl -X POST http://localhost:3000/api/smarthome/control/<device-id> \
  -H "Content-Type: application/json" \
  -d '{"state": true, "brightness": 80}'

# 发现设备
curl -X POST http://localhost:3000/api/smarthome/discover
```

## 工具列表

插件注册以下 Agent 工具：

| 工具名 | 说明 |
|--------|------|
| smart_home_list_devices | 列出所有设备 |
| smart_home_control | 控制单个设备 |
| smart_home_discover | 扫描新设备 |
| smart_home_all_on | 打开所有设备 |
| smart_home_all_off | 关闭所有设备 |

## 故障排除

### 设备发现失败

1. 确保设备和 Gateway 在同一局域网
2. 检查防火墙是否阻止 UDP 43210 端口
3. 确认设备已开机并连接到 WiFi

### 控制无响应

1. 检查设备 IP 是否变化（建议设置静态 IP）
2. 检查设备 API 服务是否正常
3. 尝试重新发现设备

### 插件未加载

1. 确认插件目录正确
2. 检查 Gateway 启动日志
3. 运行 `openclaw plugins list` 确认

## License

MIT
