# Smart Home Control Skill

极简智能家居控制系统 - OpenClaw Gateway Skill

## 功能

- 🔍 **设备发现**：自动扫描局域网内的智能设备
- 💡 **设备控制**：开关、亮度调节、命名
- 📊 **状态同步**：实时获取设备状态
- 🌐 **Web 管理界面**：嵌入 Gateway 的设备管理页面

## 安装

```bash
cd gateway-skill
pip install -r requirements.txt
```

## 配置

设备配置保存在：
```
~/.openclaw/workspace/smart-home-config.json
```

## 使用

### 自然语言命令

```
发现智能家居设备
打开客厅的灯
把卧室灯亮度调到 50%
关闭所有灯
查询客厅灯的状态
```

### HTTP API

| 端点 | 方法 | 说明 |
|------|------|------|
| `/api/smarthome/devices` | GET | 获取设备列表 |
| `/api/smarthome/devices` | POST | 添加设备 |
| `/api/smarthome/devices/:id` | DELETE | 移除设备 |
| `/api/smarthome/control/:id` | POST | 控制设备 |
| `/api/smarthome/control/all` | POST | 控制所有设备 |
| `/api/smarthome/discover` | GET | 发现新设备 |
| `/api/smarthome/sync` | POST | 同步状态 |

### Web 界面

访问 `/smarthome/` 查看 Web 管理界面。

## 嵌入 Gateway

在 OpenClaw Gateway 配置中添加：

```yaml
plugins:
  smart-home:
    enabled: true
    route: /smarthome
    api_prefix: /api/smarthome
```

## 设备协议

设备需要支持以下 API：

- `GET /api/info` - 获取设备信息
- `POST /api/control` - 控制设备
- `GET /api/state` - 获取状态
- UDP 43210 端口响应发现请求

详见 [docs/API.md](../docs/API.md)

## License

MIT
