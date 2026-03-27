#!/usr/bin/env python3
"""
Gateway API 服务器 - 异步版本
使用 Quart 框架，统一异步模型
"""

from quart import Quart, jsonify, request
from quart_cors import cors
import asyncio
import json
import socket
import aiohttp
from pathlib import Path
from datetime import datetime
from typing import Dict, Any, Optional, List

# 配置
CONFIG_FILE = Path.home() / '.openclaw' / 'workspace' / 'smart-home-config.json'
UDP_DISCOVERY_PORT = 43210
DISCOVERY_TIMEOUT = 5.0

app = Quart(__name__)
app = cors(app, allow_origin='*')

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

async def discover_devices_async(timeout: float = DISCOVERY_TIMEOUT) -> List[Dict]:
    """异步 UDP 发现设备"""
    devices = []
    loop = asyncio.get_event_loop()
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind(('0.0.0.0', UDP_DISCOVERY_PORT))
        sock.setblocking(False)
        
        # 发送广播
        discovery_msg = b'SMART_HOME_DISCOVER'
        for addr in ['255.255.255.255', '192.168.1.255', '192.168.0.255']:
            try:
                sock.sendto(discovery_msg, (addr, UDP_DISCOVERY_PORT))
            except:
                pass
        
        # 异步等待响应
        start_time = loop.time()
        while loop.time() - start_time < timeout:
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

async def get_device_info(ip: str, timeout: float = 2.0) -> Optional[Dict]:
    """异步获取设备信息"""
    try:
        timeout_config = aiohttp.ClientTimeout(total=timeout)
        async with aiohttp.ClientSession(timeout=timeout_config) as session:
            async with session.get(f'http://{ip}/api/info') as resp:
                if resp.status == 200:
                    return await resp.json()
    except:
        pass
    return None

async def control_device_async(ip: str, payload: Dict, timeout: float = 5.0) -> Dict:
    """异步控制设备"""
    try:
        timeout_config = aiohttp.ClientTimeout(total=timeout)
        async with aiohttp.ClientSession(timeout=timeout_config) as session:
            async with session.post(f'http://{ip}/api/control', json=payload) as resp:
                return await resp.json()
    except Exception as e:
        return {'success': False, 'error': str(e)}

# ========== HTTP API ==========

@app.route('/api/smarthome/devices', methods=['GET'])
async def list_devices():
    """获取设备列表"""
    config = load_config()
    return jsonify({'devices': config.get('devices', {}), 'count': len(config.get('devices', {}))})

@app.route('/api/smarthome/devices', methods=['POST'])
async def add_device():
    """添加设备"""
    data = await request.get_json()
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
async def remove_device(device_id):
    """移除设备"""
    config = load_config()
    if device_id in config.get('devices', {}):
        del config['devices'][device_id]
        save_config(config)
        return jsonify({'success': True})
    return jsonify({'error': 'device not found'}), 404

@app.route('/api/smarthome/control/<device_id>', methods=['POST'])
async def control_single_device(device_id):
    """控制单个设备"""
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
    
    payload = await request.get_json()
    result = await control_device_async(device['ip'], payload)
    
    # 更新本地状态
    if result.get('success'):
        if 'state' in payload:
            device['state'] = payload['state']
        if 'brightness' in payload:
            device['brightness'] = payload['brightness']
        if 'name' in payload:
            device['name'] = payload['name']
        save_config(config)
    
    return jsonify(result)

@app.route('/api/smarthome/control/all', methods=['POST'])
async def control_all_devices():
    """控制所有设备"""
    config = load_config()
    payload = await request.get_json()
    state = payload.get('state')
    brightness = payload.get('brightness')
    
    tasks = []
    device_ids = []
    
    for device_id, device in config.get('devices', {}).items():
        control_payload = {}
        if state is not None:
            control_payload['state'] = state
        if brightness is not None:
            control_payload['brightness'] = brightness
        tasks.append(control_device_async(device['ip'], control_payload))
        device_ids.append(device_id)
    
    results = await asyncio.gather(*tasks)
    
    # 更新状态
    for device_id, result in zip(device_ids, results):
        if result.get('success'):
            device = config['devices'][device_id]
            if state is not None:
                device['state'] = state
            if brightness is not None:
                device['brightness'] = brightness
    
    save_config(config)
    return jsonify({'success': all(r.get('success', False) for r in results), 'results': dict(zip(device_ids, [r.get('success', False) for r in results]))})

@app.route('/api/smarthome/discover', methods=['GET', 'POST'])
async def discover():
    """发现新设备"""
    devices = await discover_devices_async()
    
    # 过滤已添加的设备
    config = load_config()
    existing_ids = set(config.get('devices', {}).keys())
    new_devices = [d for d in devices if d.get('id') not in existing_ids]
    
    return jsonify({'devices': new_devices, 'count': len(new_devices)})

@app.route('/api/smarthome/sync', methods=['POST'])
async def sync_state():
    """同步所有设备状态"""
    config = load_config()
    
    tasks = []
    device_ids = []
    
    for device_id, device in config.get('devices', {}).items():
        tasks.append(get_device_info(device['ip']))
        device_ids.append(device_id)
    
    results = await asyncio.gather(*tasks)
    
    # 更新状态
    for device_id, info in zip(device_ids, results):
        if info:
            device = config['devices'][device_id]
            device['state'] = info.get('state', device['state'])
            device['brightness'] = info.get('brightness', device['brightness'])
            if info.get('name'):
                device['name'] = info['name']
    
    save_config(config)
    return jsonify({'success': True, 'synced': sum(1 for r in results if r)})

@app.route('/api/smarthome/status', methods=['GET'])
async def get_status():
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
async def serve_web(path=''):
    """服务 Web 界面"""
    from quart import send_file
    web_dir = Path(__file__).parent / 'web'
    
    if not path or path == 'index.html':
        return await send_file(web_dir / 'index.html')
    
    file_path = web_dir / path
    if file_path.exists():
        return await send_file(file_path)
    
    return jsonify({'error': 'not found'}), 404

# ========== 主入口 ==========

if __name__ == '__main__':
    print("智能家居 API 服务器启动中...")
    print("Web 界面: http://localhost:5000/smarthome/")
    print("API 文档: http://localhost:5000/api/smarthome/status")
    app.run(host='0.0.0.0', port=5000, debug=True)
