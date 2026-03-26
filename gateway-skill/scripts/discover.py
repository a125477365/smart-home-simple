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
import subprocess
import ipaddress

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


async def discover_devices(timeout: float = DISCOVERY_TIMEOUT) -> List[Device]:
    """
    通过 UDP 广播发现局域网内的设备
    
    Args:
        timeout: 发现超时时间（秒）
    
    Returns:
        发现的设备列表
    """
    devices = []
    
    # 获取本机 IP 和网段
    local_ip = get_local_ip()
    if not local_ip:
        print("无法获取本机 IP")
        return devices
    
    # 创建 UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        # 绑定并监听
        sock.bind(('0.0.0.0', UDP_DISCOVERY_PORT))
        sock.setblocking(False)
        
        # 发送广播发现请求
        discovery_msg = b'SMART_HOME_DISCOVER'
        
        # 向广播地址发送
        for addr in get_broadcast_addresses():
            try:
                sock.sendto(discovery_msg, (addr, UDP_DISCOVERY_PORT))
            except Exception as e:
                print(f"发送到 {addr} 失败: {e}")
        
        # 异步接收响应
        loop = asyncio.get_event_loop()
        
        async def receive_responses():
            while True:
                try:
                    data, addr = await loop.sock_recvfrom(sock, 1024)
                    try:
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
                        # 去重
                        if not any(d.id == device.id for d in devices):
                            devices.append(device)
                            print(f"发现设备: {device.name} ({device.ip})")
                    except json.JSONDecodeError:
                        pass
                except Exception:
                    break
        
        # 等待响应
        try:
            await asyncio.wait_for(receive_responses(), timeout)
        except asyncio.TimeoutError:
            pass
            
    finally:
        sock.close()
    
    return devices


async def discover_by_scan(timeout: float = 2.0) -> List[Device]:
    """
    通过 IP 扫描发现设备（备用方案）
    
    扫描本网段内可能的设备
    """
    devices = []
    local_ip = get_local_ip()
    if not local_ip:
        return devices
    
    # 获取网段
    parts = local_ip.split('.')
    base = '.'.join(parts[:3])
    
    async def check_device(ip: str) -> Optional[Device]:
        try:
            async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=timeout)) as session:
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
    
    # 并发扫描
    tasks = []
    for i in range(1, 255):
        ip = f'{base}.{i}'
        if ip != local_ip:  # 跳过本机
            tasks.append(check_device(ip))
    
    results = await asyncio.gather(*tasks)
    for device in results:
        if device:
            devices.append(device)
    
    return devices


def get_local_ip() -> Optional[str]:
    """获取本机 IP 地址"""
    try:
        # 创建 UDP socket 连接外部地址
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


async def main():
    """测试入口"""
    print("正在发现设备...")
    
    # 先尝试 UDP 发现
    devices = await discover_devices(timeout=5.0)
    
    if not devices:
        print("UDP 发现未找到设备，尝试 IP 扫描...")
        devices = await discover_by_scan()
    
    print(f"\n发现 {len(devices)} 个设备:")
    for device in devices:
        status = "开" if device.state else "关"
        print(f"  - {device.name} ({device.ip}): {status}, 亮度 {device.brightness}%")
    
    return devices


if __name__ == '__main__':
    asyncio.run(main())
