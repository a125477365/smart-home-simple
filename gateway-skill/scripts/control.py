#!/usr/bin/env python3
"""
智能家居设备控制脚本
控制单个或多个设备
"""

import asyncio
import aiohttp
import json
import os
from typing import List, Optional, Dict, Any
from dataclasses import dataclass, asdict
from pathlib import Path

# 配置文件路径
CONFIG_FILE = Path.home() / '.openclaw' / 'workspace' / 'smart-home-config.json'


@dataclass
class Device:
    id: str
    name: str
    type: str
    ip: str
    version: str = '1.0.0'
    state: bool = False
    brightness: int = 100
    room: str = ''


class DeviceController:
    """设备控制器"""
    
    def __init__(self):
        self.devices: Dict[str, Device] = {}
        self.load_config()
    
    def load_config(self):
        """加载配置"""
        if CONFIG_FILE.exists():
            try:
                with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    for device_data in data.get('devices', []):
                        device = Device(**device_data)
                        self.devices[device.id] = device
            except Exception as e:
                print(f"加载配置失败: {e}")
    
    def save_config(self):
        """保存配置"""
        CONFIG_FILE.parent.mkdir(parents=True, exist_ok=True)
        data = {
            'devices': [asdict(d) for d in self.devices.values()]
        }
        with open(CONFIG_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    
    def add_device(self, device: Device):
        """添加设备"""
        self.devices[device.id] = device
        self.save_config()
    
    def remove_device(self, device_id: str):
        """移除设备"""
        if device_id in self.devices:
            del self.devices[device_id]
            self.save_config()
    
    async def get_device_info(self, ip: str) -> Optional[Dict[str, Any]]:
        """获取设备信息"""
        try:
            async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=5)) as session:
                async with session.get(f'http://{ip}/api/info') as resp:
                    if resp.status == 200:
                        return await resp.json()
        except Exception as e:
            print(f"获取设备信息失败: {e}")
        return None
    
    async def control_device(self, device_id: str, state: Optional[bool] = None,
                             brightness: Optional[int] = None,
                             name: Optional[str] = None) -> bool:
        """控制设备"""
        device = self.devices.get(device_id)
        if not device:
            # 尝试通过名称查找
            for d in self.devices.values():
                if d.name == device_id or device_id in d.name:
                    device = d
                    break
        
        if not device:
            print(f"设备不存在: {device_id}")
            return False
        
        payload = {}
        if state is not None:
            payload['state'] = state
        if brightness is not None:
            payload['brightness'] = brightness
        if name is not None:
            payload['name'] = name
        
        try:
            async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=5)) as session:
                async with session.post(
                    f'http://{device.ip}/api/control',
                    json=payload
                ) as resp:
                    if resp.status == 200:
                        result = await resp.json()
                        if result.get('success'):
                            # 更新本地状态
                            if state is not None:
                                device.state = state
                            if brightness is not None:
                                device.brightness = brightness
                            if name is not None:
                                device.name = name
                            self.save_config()
                            return True
        except Exception as e:
            print(f"控制设备失败: {e}")
        
        return False
    
    async def control_all(self, state: Optional[bool] = None,
                          brightness: Optional[int] = None) -> Dict[str, bool]:
        """控制所有设备"""
        results = {}
        tasks = []
        for device_id in self.devices:
            tasks.append(self.control_device(device_id, state, brightness))
        
        responses = await asyncio.gather(*tasks)
        for device_id, success in zip(self.devices.keys(), responses):
            results[device_id] = success
        
        return results
    
    async def sync_state(self) -> None:
        """同步所有设备状态"""
        async def sync_one(device: Device):
            info = await self.get_device_info(device.ip)
            if info:
                device.state = info.get('state', device.state)
                device.brightness = info.get('brightness', device.brightness)
                if info.get('name'):
                    device.name = info['name']
        
        await asyncio.gather(*[sync_one(d) for d in self.devices.values()])
        self.save_config()
    
    def find_by_name(self, name: str) -> List[Device]:
        """按名称查找设备"""
        name_lower = name.lower()
        return [d for d in self.devices.values() 
                if name_lower in d.name.lower() or name_lower in d.room.lower()]


# ========== 便捷函数 ==========

_controller: Optional[DeviceController] = None


def get_controller() -> DeviceController:
    global _controller
    if _controller is None:
        _controller = DeviceController()
    return _controller


async def turn_on(device_id: str = None) -> bool:
    """打开设备"""
    c = get_controller()
    if device_id:
        return await c.control_device(device_id, state=True)
    else:
        results = await c.control_all(state=True)
        return all(results.values())


async def turn_off(device_id: str = None) -> bool:
    """关闭设备"""
    c = get_controller()
    if device_id:
        return await c.control_device(device_id, state=False)
    else:
        results = await c.control_all(state=False)
        return all(results.values())


async def set_brightness(device_id: str, brightness: int) -> bool:
    """设置亮度"""
    c = get_controller()
    return await c.control_device(device_id, brightness=brightness)


async def rename_device(device_id: str, name: str) -> bool:
    """重命名设备"""
    c = get_controller()
    return await c.control_device(device_id, name=name)


async def get_status(device_id: str) -> Optional[Dict[str, Any]]:
    """获取设备状态"""
    c = get_controller()
    device = c.devices.get(device_id)
    if not device:
        return None
    
    async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=5)) as session:
        async with session.get(f'http://{device.ip}/api/state') as resp:
            if resp.status == 200:
                return await resp.json()
    return None


# ========== 主入口 ==========

async def main():
    """测试入口"""
    import sys
    
    c = get_controller()
    
    if len(sys.argv) < 2:
        print("用法: control.py <command> [args]")
        print("命令: on, off, brightness, status, list, sync")
        return
    
    cmd = sys.argv[1]
    
    if cmd == 'list':
        print("已配置设备:")
        for device in c.devices.values():
            status = "开" if device.state else "关"
            print(f"  {device.name} ({device.ip}): {status}, 亮度 {device.brightness}%")
    
    elif cmd == 'sync':
        await c.sync_state()
        print("状态已同步")
    
    elif cmd == 'on':
        device_id = sys.argv[2] if len(sys.argv) > 2 else None
        if device_id:
            success = await turn_on(device_id)
            print(f"打开 {device_id}: {'成功' if success else '失败'}")
        else:
            results = await c.control_all(state=True)
            print(f"已打开所有设备: {sum(results.values())}/{len(results)}")
    
    elif cmd == 'off':
        device_id = sys.argv[2] if len(sys.argv) > 2 else None
        if device_id:
            success = await turn_off(device_id)
            print(f"关闭 {device_id}: {'成功' if success else '失败'}")
        else:
            results = await c.control_all(state=False)
            print(f"已关闭所有设备: {sum(results.values())}/{len(results)}")
    
    elif cmd == 'brightness':
        if len(sys.argv) < 4:
            print("用法: control.py brightness <device> <value>")
            return
        device_id = sys.argv[2]
        value = int(sys.argv[3])
        success = await set_brightness(device_id, value)
        print(f"设置亮度: {'成功' if success else '失败'}")
    
    else:
        print(f"未知命令: {cmd}")


if __name__ == '__main__':
    asyncio.run(main())
