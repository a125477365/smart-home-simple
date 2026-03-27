# OpenClaw Gateway 集成指南

## 概述

本指南说明如何将智能家居系统原生集成到 OpenClaw Gateway，并复用 Gateway 的认证机制。

## 架构

```
┌─────────────────────────────────────────────────────────────┐
│                    OpenClaw Gateway                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │  Control UI │  │  Agent API  │  │  Plugin System      │ │
│  │  (认证中心) │  │  Tool Calls │  │  smart-home 插件    │ │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│         │ 认证           │                     │            │
│         │                │                     │            │
│         └────────────────┴─────────────────────┘            │
│                           │                                  │
└───────────────────────────┼──────────────────────────────────┘
                            │
              ┌─────────────┴─────────────┐
              │   认证检查流程            │
              │   1. WebSocket 连接验证   │
              │   2. Token 校验           │
              │   3. 来源 IP 检查         │
              └─────────────┬─────────────┘
                            │
                            ▼
              ┌────────────────────────┐
              │  局域网智能设备         │
              │  (ESP32 + REST API)    │
              └────────────────────────┘
```

## 认证机制

智能家居插件**复用 Gateway 的认证系统**：

### 前端认证检查
1. 页面加载时尝试连接 Gateway WebSocket
2. 如果 WebSocket 连接成功，说明已通过 Gateway 认证
3. 如果连接失败（code 1008），说明未认证，显示登录提示

### 后端 API 保护
所有 API 请求都会验证：
- **来源检查**：请求必须来自 Gateway Control UI
- **Token 验证**：非本地请求需要有效的 Bearer Token

### 认证流程
```
用户 → Gateway Control UI → 登录认证
        ↓
      WebSocket 连接成功
        ↓
      点击"智能家居" → /smarthome
        ↓
      后端检查 referer + cookie + token
        ↓
      ├─ 已认证 → 返回控制面板
      └─ 未认证 → 返回认证提示页
        ↓
      API 请求携带认证信息
        ↓
      后端验证来源 + Token
```

### 安全检查层级

| 检查项 | 时机 | 说明 |
|--------|------|------|
| **Referer 检查** | 页面加载 | 必须从 Gateway Control UI 跳转 |
| **Cookie 检查** | 页面加载 | 检查 Gateway 会话 Cookie |
| **Token 检查** | API 调用 | URL 参数或 Authorization header |
| **IP 检查** | 所有请求 | 本地回环地址自动放行 |

### 直接访问会怎样？

如果用户直接访问 `http://localhost:18789/smarthome`（不通过 Gateway）：

1. **后端检查 Referer** → 为空，不匹配
2. **后端检查 Cookie** → 没有 Gateway 会话
3. **后端检查 IP** → 如果是本地可能放行（取决于配置）
4. **后端检查 Token** → 没有 Token
5. **结果**：返回"需要认证"页面，而不是控制面板

**页面内容只显示：**
```
🔐 需要认证
智能家居控制面板需要先登录 Gateway
[前往 Gateway 登录]
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

### 1. 通过 Control UI 导航

安装插件后，Gateway Control UI 会新增 **"智能家居"** 导航入口：

```
http://localhost:18789/
      ↓
   Control UI
      ↓
  点击 "智能家居" 链接
      ↓
http://localhost:18789/smarthome
```

**功能：**
- 设备列表展示
- 开关控制
- 亮度调节
- 设备发现
- 重命名/移除
- 自动刷新状态

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
curl http://localhost:18789/api/smarthome/devices

# 控制设备
curl -X POST http://localhost:18789/api/smarthome/control/<device-id> \
  -H "Content-Type: application/json" \
  -d '{"state": true, "brightness": 80}'

# 发现设备
curl -X POST http://localhost:18789/api/smarthome/discover

# 同步状态
curl -X POST http://localhost:18789/api/smarthome/sync
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

## 导航集成

插件通过 `api.registerNavLink()` 在 Control UI 中注册导航入口：

```typescript
api.registerNavLink({
  id: 'smart-home',
  label: '智能家居',
  icon: 'home',
  path: '/smarthome',
  order: 100
});
```

用户在 Control UI 可以：
1. 看到侧边栏的 "智能家居" 入口
2. 点击后跳转到控制面板
3. 控制面板有 "返回 Gateway" 链接

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

### 导航链接不显示

1. 确认插件已正确编译（`pnpm build`）
2. 重启 Gateway
3. 检查 `api.registerNavLink` 是否被调用

## License

MIT
