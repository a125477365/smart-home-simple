/**
 * 设备管理器 - 核心逻辑
 */

import dgram from 'dgram';
import http from 'http';
import fs from 'fs';
import path from 'path';
import type { SmartHomeConfig } from './index.js';

export interface Device {
  id: string;
  name: string;
  type: string;
  ip: string;
  version: string;
  state: boolean;
  brightness: number;
  room?: string;
  addedAt?: string;
}

export interface DeviceConfig {
  devices: Record<string, Device>;
}

export class DeviceManager {
  private config: SmartHomeConfig;
  private configPath: string;
  private deviceConfig: DeviceConfig;
  private discoverTimer?: NodeJS.Timeout;

  constructor(config: SmartHomeConfig) {
    this.config = config;
    this.configPath = path.join(
      process.env.HOME || '/root',
      '.openclaw/workspace/smart-home-config.json'
    );
    this.deviceConfig = this.loadConfig();
  }

  private loadConfig(): DeviceConfig {
    try {
      if (fs.existsSync(this.configPath)) {
        const data = fs.readFileSync(this.configPath, 'utf-8');
        return JSON.parse(data);
      }
    } catch (err) {
      console.error('[SmartHome] Load config error:', err);
    }
    return { devices: {} };
  }

  private saveConfig(): void {
    try {
      fs.mkdirSync(path.dirname(this.configPath), { recursive: true });
      fs.writeFileSync(this.configPath, JSON.stringify(this.deviceConfig, null, 2));
    } catch (err) {
      console.error('[SmartHome] Save config error:', err);
    }
  }

  async listDevices(): Promise<{ devices: Record<string, Device>; count: number }> {
    return {
      devices: this.deviceConfig.devices,
      count: Object.keys(this.deviceConfig.devices).length
    };
  }

  async discoverDevices(timeout: number = 5): Promise<Device[]> {
    return new Promise((resolve) => {
      const devices: Device[] = [];
      const socket = dgram.createSocket('udp4');

      socket.bind(() => {
        socket.setBroadcast(true);
        socket.setReuseAddress(true);

        // 发送发现请求
        const msg = Buffer.from('SMART_HOME_DISCOVER');
        socket.send(msg, 43210, '255.255.255.255');
        socket.send(msg, 43210, '192.168.1.255');
        socket.send(msg, 43210, '192.168.0.255');
      });

      socket.on('message', (data, rinfo) => {
        try {
          const info = JSON.parse(data.toString());
          if (!devices.find(d => d.id === info.id)) {
            devices.push({
              id: info.id,
              name: info.name || 'Unknown',
              type: info.type || 'unknown',
              ip: info.ip || rinfo.address,
              version: info.version || '1.0.0',
              state: info.state || false,
              brightness: info.brightness || 100
            });
          }
        } catch {
          // 忽略无效响应
        }
      });

      setTimeout(() => {
        socket.close();
        resolve(devices);
      }, timeout * 1000);
    });
  }

  async controlDevice(
    deviceId: string,
    params: { state?: boolean; brightness?: number; name?: string }
  ): Promise<{ success: boolean; error?: string; device?: Device }> {
    // 先查找设备
    let device = this.deviceConfig.devices[deviceId];

    // 按名称查找
    if (!device) {
      for (const d of Object.values(this.deviceConfig.devices)) {
        if (d.name.includes(deviceId)) {
          device = d;
          deviceId = d.id;
          break;
        }
      }
    }

    if (!device) {
      return { success: false, error: 'Device not found' };
    }

    try {
      const payload: Record<string, unknown> = {};
      if (params.state !== undefined) payload.state = params.state;
      if (params.brightness !== undefined) payload.brightness = params.brightness;
      if (params.name !== undefined) payload.name = params.name;

      const result = await this.httpPost(device.ip, '/api/control', payload);

      if (result.success) {
        // 更新本地状态
        if (params.state !== undefined) device.state = params.state;
        if (params.brightness !== undefined) device.brightness = params.brightness;
        if (params.name !== undefined) device.name = params.name;
        this.saveConfig();
      }

      return { success: true, device };
    } catch (err) {
      return { success: false, error: String(err) };
    }
  }

  async controlAll(
    params: { state?: boolean; brightness?: number }
  ): Promise<{ success: number; failed: number; total: number }> {
    const deviceIds = Object.keys(this.deviceConfig.devices);
    let success = 0;
    let failed = 0;

    for (const deviceId of deviceIds) {
      const result = await this.controlDevice(deviceId, params);
      if (result.success) success++;
      else failed++;
    }

    return { success, failed, total: deviceIds.length };
  }

  async addDevice(device: Device): Promise<Device> {
    device.addedAt = new Date().toISOString();
    this.deviceConfig.devices[device.id] = device;
    this.saveConfig();
    return device;
  }

  async removeDevice(deviceId: string): Promise<void> {
    delete this.deviceConfig.devices[deviceId];
    this.saveConfig();
  }

  startAutoDiscover(interval: number): void {
    this.discoverTimer = setInterval(async () => {
      console.log('[SmartHome] Auto discover...');
      const devices = await this.discoverDevices(3);

      // 自动添加新设备
      for (const device of devices) {
        if (!this.deviceConfig.devices[device.id]) {
          console.log(`[SmartHome] Found new device: ${device.name} (${device.ip})`);
          await this.addDevice(device);
        }
      }
    }, interval);
  }

  stopAutoDiscover(): void {
    if (this.discoverTimer) {
      clearInterval(this.discoverTimer);
      this.discoverTimer = undefined;
    }
  }

  async getWebPage(): Promise<string> {
    // 返回嵌入式 Web 界面
    const webPath = path.join(__dirname, 'web', 'panel.html');
    if (fs.existsSync(webPath)) {
      return fs.readFileSync(webPath, 'utf-8');
    }
    return '<html><body><h1>Smart Home Panel</h1><p>Web interface not found</p></body></html>';
  }

  private httpPost(ip: string, path: string, data: unknown): Promise<Record<string, unknown>> {
    return new Promise((resolve, reject) => {
      const payload = JSON.stringify(data);
      const req = http.request(
        {
          hostname: ip,
          port: 80,
          path,
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(payload)
          },
          timeout: 5000
        },
        (res) => {
          let body = '';
          res.on('data', chunk => body += chunk);
          res.on('end', () => {
            try {
              resolve(JSON.parse(body));
            } catch {
              reject(new Error('Invalid response'));
            }
          });
        }
      );

      req.on('error', reject);
      req.on('timeout', () => {
        req.destroy();
        reject(new Error('Timeout'));
      });

      req.write(payload);
      req.end();
    });
  }
}
