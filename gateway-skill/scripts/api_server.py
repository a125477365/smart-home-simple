#!/usr/bin/env python3
"""
Gateway API 服务器
提供设备管理 HTTP API，嵌入 OpenClaw Gateway
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import asyncio
import json
import os
from pathlib import Path
from datetime import datetime
from typing import Dict, Any, Optional

# 配置文件路径
CONFIG_FILE = Path.home() / '.openclaw' / 'workspace' / 'smart-home-config.json'

app = Flask(__name__)
CORS(app)

# ========== 配置管理 ==========

def load_config() -> Dict[str, Any]:
    """加载配置"""
    if CONFIG_FILE.exists():
        with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
            return json.load(f)
    return {'devices': {}}

def save_config(config: Dict[str, Any]):
    """保存配置"""
    CONFIG_FILE.parent.mkdir(parents=True, exist_ok=True)
    with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
        json.dump(config, f, ensure_ascii=False, indent=2)

# ========== 设备发现 ==========

async def discover_devices_async(timeout: float = 5.0) -> list:
    """异步发现设备"""
    import socket
    
    devices = []
    discovery_port = 43210
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind(('0.0.0.0', discovery_port))
        sock.setblocking(False)
        
        # 发送发现请求
        discovery_msg = b'SMART_HOME_DISCOVER'
        for addr in ['255.255.255.255', '192.168.1.255', '192.168.0.255']:
            try:
                sock.sendto(discovery_msg, (addr, discovery_port))
            except:
                pass
        
        # 等待响应
        loop = asyncio.get_event_loop()
        start_time = asyncio.get_event_loop().time()
        
        while asyncio.get_event_loop().time() - start_time < timeout:
            try:
                await asyncio.sleep(0.1)
                while True:
                    try:
                        data, addr = sock.recvfrom(1024)
                        info = json.loads(data.decode())
                        if not any(d.get('id') == info.get('id') for d in devices):
                            devices.append(info)
                    except socket.error:
                        break
                    except json.JSONDecodeError:
                        continue
            except:
                break
    finally:
        sock.close()
    
    return devices

def run_async(coro):
    """在同步环境中运行异步函数"""
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        return loop.run_until_complete(coro)
    finally:
        loop.close()

# ========== HTTP API ==========

@app.route('/api/smarthome/devices', methods=['GET'])
def list_devices():
    """获取设备列表"""
    config = load_config()
    return jsonify({'devices': config.get('devices', {}), 'count': len(config.get('devices', {}))})

@app.route('/api/smarthome/devices', methods=['POST'])
def add_device():
    """添加设备"""
    data = request.json
    if not data or 'id' not in data:
        return jsonify({'error': 'missing device id'}), 400
    
    config = load_config()
    config.setdefault('devices', {})
    
    device_id = data['id']
    config['devices'][device_id] = {
        'id': device_id,
        'ip': data.get('ip', ''),
        'name': data.get('name', f'设备 {device_id[:6]}'),
        'type': data.get('type', 'unknown'),
        'version': data.get('version', '1.0.0'),
        'state': False,
        'brightness': 100,
        'room': data.get('room', ''),
        'added_at': datetime.now().isoformat()
    }
    
    save_config(config)
    return jsonify({'success': True, 'device': config['devices'][device_id]})

@app.route('/api/smarthome/devices/<device_id>', methods=['DELETE'])
def remove_device(device_id):
    """移除设备"""
    config = load_config()
    if device_id in config.get('devices', {}):
        del config['devices'][device_id]
        save_config(config)
        return jsonify({'success': True})
    return jsonify({'error': 'device not found'}), 404

@app.route('/api/smarthome/control/<device_id>', methods=['POST'])
def control_device(device_id):
    """控制设备"""
    import requests
    
    config = load_config()
    device = config.get('devices', {}).get(device_id)
    
    if not device:
        # 尝试通过名称查找
        for d in config.get('devices', {}).values():
            if device_id in d.get('name', ''):
                device = d
                device_id = d['id']
                break
    
    if not device:
        return jsonify({'error': 'device not found'}), 404
    
    try:
        resp = requests.post(
            f"http://{device['ip']}/api/control",
            json=request.json,
            timeout=5
        )
        result = resp.json()
        
        # 更新本地状态
        if result.get('success'):
            if 'state' in request.json:
                device['state'] = request.json['state']
            if 'brightness' in request.json:
                device['brightness'] = request.json['brightness']
            if 'name' in request.json:
                device['name'] = request.json['name']
            save_config(config)
        
        return jsonify(result)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/smarthome/control/all', methods=['POST'])
def control_all():
    """控制所有设备"""
    import requests
    from concurrent.futures import ThreadPoolExecutor
    
    config = load_config()
    state = request.json.get('state')
    brightness = request.json.get('brightness')
    
    results = {}
    
    def control_one(device_id, device):
        try:
            payload = {}
            if state is not None:
                payload['state'] = state
            if brightness is not None:
                payload['brightness'] = brightness
            
            resp = requests.post(
                f"http://{device['ip']}/api/control",
                json=payload,
                timeout=5
            )
            return resp.json().get('success', False)
        except:
            return False
    
    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = {}
        for device_id, device in config.get('devices', {}).items():
            futures[executor.submit(control_one, device_id, device)] = device_id
        
        for future in futures:
            results[futures[future]] = future.result()
    
    # 更新状态
    for device_id, device in config.get('devices', {}).items():
        if results.get(device_id):
            if state is not None:
                device['state'] = state
            if brightness is not None:
                device['brightness'] = brightness
    save_config(config)
    
    return jsonify({'success': all(results.values()), 'results': results})

@app.route('/api/smarthome/discover', methods=['POST', 'GET'])
def discover():
    """发现新设备"""
    devices = run_async(discover_devices_async(5.0))
    
    # 过滤已添加的设备
    config = load_config()
    existing_ids = set(config.get('devices', {}).keys())
    new_devices = [d for d in devices if d.get('id') not in existing_ids]
    
    return jsonify({'devices': new_devices, 'count': len(new_devices)})

@app.route('/api/smarthome/sync', methods=['POST'])
def sync_state():
    """同步所有设备状态"""
    import requests
    from concurrent.futures import ThreadPoolExecutor
    
    config = load_config()
    
    def sync_one(device_id, device):
        try:
            resp = requests.get(f"http://{device['ip']}/api/info", timeout=5)
            info = resp.json()
            device['state'] = info.get('state', device['state'])
            device['brightness'] = info.get('brightness', device['brightness'])
            if info.get('name'):
                device['name'] = info['name']
            return True
        except:
            return False
    
    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = {executor.submit(sync_one, did, d): did for did, d in config.get('devices', {}).items()}
        results = {futures[f]: f.result() for f in futures}
    
    save_config(config)
    return jsonify({'success': True, 'synced': sum(results.values())})

@app.route('/api/smarthome/status', methods=['GET'])
def get_status():
    """获取系统状态"""
    config = load_config()
    devices = config.get('devices', {})
    on_count = sum(1 for d in devices.values() if d.get('state'))
    
    return jsonify({
        'total_devices': len(devices),
        'on_devices': on_count,
        'off_devices': len(devices) - on_count
    })

# ========== 静态文件服务 ==========

@app.route('/smarthome/')
@app.route('/smarthome/<path:path>')
def serve_web(path=''):
    """服务 Web 界面"""
    from flask import send_file
    web_dir = Path(__file__).parent / 'web'
    
    if not path or path == 'index.html':
        return send_file(web_dir / 'index.html')
    
    file_path = web_dir / path
    if file_path.exists():
        return send_file(file_path)
    
    return jsonify({'error': 'not found'}), 404

# ========== 主入口 ==========

if __name__ == '__main__':
    print("智能家居 API 服务器启动中...")
    print("Web 界面: http://localhost:5000/smarthome/")
    print("API 文档: http://localhost:5000/api/smarthome/status")
    app.run(host='0.0.0.0', port=5000, debug=True)
