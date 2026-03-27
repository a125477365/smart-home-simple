#!/usr/bin/env python3
"""
智能家居设备发现脚本
扫描局域网内的 Smart Home 设备
"""

import socket
import json
import asyncio
import aiohttp
from typing import List, Dict, Optional
from dataclasses import dataclass

UDP_DISCOVERY_PORT = 43210
DISCOVERY_TIMEOUT = 5.0

@dataclass
class Device:
    """设备信息"""
    id: str
    name: str
    type: str
    ip: str
    version: str
    state: bool = False
    brightness: int = 100

def get_local_ip() -> Optional[str]:
    """获取本机 IP 地址"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return None

def get_broadcast_addresses() -> List[str]:
    """获取广播地址列表"""
    addresses = ['255.255.255.255']
    local_ip = get_local_ip()
    if local_ip:
        parts = local_ip.split('.')
        addresses.append(f'{parts[0]}.{parts[1]}.{parts[2]}.255')
    return addresses

async def discover_by_udp(timeout: float = DISCOVERY_TIMEOUT) -> List[Device]:
    """UDP 广播发现"""
    devices = []
    loop = asyncio.get_event_loop()
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind(('0.0.0.0', UDP_DISCOVERY_PORT))
        sock.setblocking(False)
        
        # 发送广播
        for addr in get_broadcast_addresses():
            try:
                sock.sendto(b'SMART_HOME_DISCOVER', (addr, UDP_DISCOVERY_PORT))
            except Exception as e:
                print(f"发送到 {addr} 失败: {e}")
        
        # 接收响应
        start_time = loop.time()
        while loop.time() - start_time < timeout:
            try:
                await asyncio.sleep(0.1)
                while True:
                    try:
                        data, addr = sock.recvfrom(1024)
                        info = json.loads(data.decode())
                        device = Device(
                            id=info.get('id', ''),
                            name=info.get('name', 'Unknown'),
                            type=info.get('type', 'unknown'),
                            ip=info.get('ip', addr[0]),
                            version=info.get('version', '1.0.0'),
                            state=info.get('state', False),
                            brightness=info.get('brightness', 100)
                        )
                        if not any(d.id == device.id for d in devices):
                            devices.append(device)
                            print(f"发现设备: {device.name} ({device.ip})")
                    except socket.error:
                        break
                    except json.JSONDecodeError:
                        continue
            except:
                break
    finally:
        sock.close()
    
    return devices

async def discover_by_scan(timeout: float = 2.0) -> List[Device]:
    """IP 扫描发现（备用方案）"""
    devices = []
    local_ip = get_local_ip()
    if not local_ip:
        return devices
    
    parts = local_ip.split('.')
    base = '.'.join(parts[:3])
    
    async def check_device(ip: str) -> Optional[Device]:
        try:
            timeout_config = aiohttp.ClientTimeout(total=timeout)
            async with aiohttp.ClientSession(timeout=timeout_config) as session:
                async with session.get(f'http://{ip}/api/info') as resp:
                    if resp.status == 200:
                        info = await resp.json()
                        return Device(
                            id=info.get('id', ''),
                            name=info.get('name', 'Unknown'),
                            type=info.get('type', 'unknown'),
                            ip=ip,
                            version=info.get('version', '1.0.0'),
                            state=info.get('state', False),
                            brightness=info.get('brightness', 100)
                        )
        except:
            pass
        return None
    
    tasks = [check_device(f'{base}.{i}') for i in range(1, 255) if f'{base}.{i}' != local_ip]
    results = await asyncio.gather(*tasks)
    
    for device in results:
        if device:
            devices.append(device)
    
    return devices

async def main():
    """测试入口"""
    print("正在发现设备...")
    
    # 先尝试 UDP 发现
    devices = await discover_by_udp(timeout=5.0)
    
    if not devices:
        print("UDP 发现未找到设备，尝试 IP 扫描...")
        devices = await discover_by_scan()
    
    print(f"\n发现 {len(devices)} 个设备:")
    for device in devices:
        status = "开" if device.state else "关"
        print(f" - {device.name} ({device.ip}): {status}, 亮度 {device.brightness}%")
    
    return devices

if __name__ == '__main__':
    asyncio.run(main())
