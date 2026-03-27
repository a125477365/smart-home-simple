# OpenClaw Gateway 集成指南

## 概述

本指南说明如何将智能家居系统集成到 OpenClaw Gateway。

## 架构

```
┌──────────────────────────────────────────────────────┐
│                   OpenClaw Gateway                    │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │  Web UI     │  │  API Server │  │  Skill      │  │
│  │ /smarthome/ │  │  Quart App  │  │  自然语言   │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         │                │                │          │
│         └────────────────┴────────────────┘          │
│                          │                            │
└──────────────────────────┼────────────────────────────┘
                           │ HTTP API
                           ▼
              ┌────────────────────────┐
              │   局域网智能设备       │
              │   (ESP32 + REST API)   │
              └────────────────────────┘
```

## 部署方式

### 方式一：独立进程

```bash
# 安装依赖
cd gateway-skill
pip install -r requirements.txt

# 启动服务
python scripts/api_server.py

# 服务将运行在 http://localhost:5000
```

### 方式二：嵌入 Gateway

在 OpenClaw Gateway 的 `plugins` 目录创建软链接：

```bash
ln -s /path/to/smart-home-simple/gateway-skill ~/.openclaw/plugins/smart-home
```

在 Gateway 配置中启用：

```yaml
# ~/.openclaw/config/gateway.yaml
plugins:
  entries:
    smart-home:
      enabled: true
      module: smart-home.scripts.api_server:app
      route: /smarthome
```

### 方式三：Systemd 服务

```bash
# 创建服务文件
sudo cat > /etc/systemd/system/smarthome.service << EOF
[Unit]
Description=Smart Home API Server
After=network.target

[Service]
Type=simple
User=node
WorkingDirectory=/home/node/.openclaw/workspace/smart-home-simple/gateway-skill
ExecStart=/usr/bin/python3 scripts/api_server.py
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# 启动服务
sudo systemctl enable smarthome
sudo systemctl start smarthome
```

## API 代理配置

如需通过 Gateway 统一入口访问，配置反向代理：

```yaml
# OpenClaw Gateway 路由配置
routes:
  /api/smarthome:
    proxy: http://localhost:5000/api/smarthome
  /smarthome:
    proxy: http://localhost:5000/smarthome
```

## 安全配置

### 1. 限制访问 IP

```python
# api_server.py 中添加
from quart import request, abort

ALLOWED_IPS = ['127.0.0.1', '192.168.1.0/24']

@app.before_request
def limit_remote_addr():
    client_ip = request.remote_addr
    if not ip_in_range(client_ip, ALLOWED_IPS):
        abort(403)
```

### 2. API Token（可选）

```python
# 添加 Token 认证
API_TOKEN = os.environ.get('SMARTHOME_TOKEN', 'your-secret-token')

@app.before_request
def check_token():
    if request.endpoint in ['discover', 'control_all']:
        token = request.headers.get('X-API-Token')
        if token != API_TOKEN:
            abort(401)
```

## 自然语言 Skill 集成

在 OpenClaw 中注册自然语言处理器：

```python
# ~/.openclaw/skills/smart-home-nlp/SKILL.md

触发词：智能家居、灯、插座

意图识别：
- 打开/关闭 → 调用 control API
- 发现 → 调用 discover API
- 状态 → 调用 status API
- 亮度 → 调用 control API (brightness)
```

## 测试

```bash
# 测试 API
curl http://localhost:5000/api/smarthome/status

# 发现设备
curl -X POST http://localhost:5000/api/smarthome/discover

# 控制设备
curl -X POST http://localhost:5000/api/smarthome/control/<device_id> \
  -H "Content-Type: application/json" \
  -d '{"state": true}'
```

## 故障排除

### 设备发现失败

1. 检查设备是否开机
2. 检查设备是否在同一局域网
3. 检查 UDP 43210 端口是否被防火墙阻止

### API 无法访问设备

1. 检查设备 IP 是否正确
2. 检查设备 API 服务是否运行
3. 检查网络连通性：`ping <device-ip>`

### Web 界面空白

1. 检查 API 服务是否启动
2. 检查浏览器控制台错误
3. 检查 CORS 配置
