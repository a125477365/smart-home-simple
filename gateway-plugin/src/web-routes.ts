/**
 * Gateway Web 路由（备用方案 - 如果 API 不可用）
 */
import type { Request, Response } from 'http';
import type { DeviceManager } from './device-manager.js';
import fs from 'fs';
import path from 'path';

export function createWebRoutes(deviceManager: DeviceManager) {
  return {
    async handleGetSmarthome(req: Request, res: Response): Promise<void> {
      res.setHeader('Content-Type', 'text/html; charset=utf-8');
      const html = await deviceManager.getWebPage();
      res.send(html);
    },

    async handleGetDevices(req: Request, res: Response): Promise<void> {
      const devices = await deviceManager.listDevices();
      res.json(devices);
    },

    async handlePostDiscover(req: Request, res: Response): Promise<void> {
      const devices = await deviceManager.discoverDevices(5);
      res.json({ devices, count: devices.length });
    },

    async handlePostControl(req: Request, res: Response): Promise<void> {
      const deviceId = req.params?.deviceId;
      const body = req.body || {};
      const result = await deviceManager.controlDevice(deviceId, body);
      res.json(result);
    },

    async handlePostControlAll(req: Request, res: Response): Promise<void> {
      const body = req.body || {};
      const result = await deviceManager.controlAll(body);
      res.json(result);
    },

    async handleDeleteDevice(req: Request, res: Response): Promise<void> {
      const deviceId = req.params?.deviceId;
      await deviceManager.removeDevice(deviceId);
      res.json({ success: true });
    },

    async handlePostSync(req: Request, res: Response): Promise<void> {
      // 同步所有设备状态
      const { devices } = await deviceManager.listDevices();
      let synced = 0;

      for (const device of Object.values(devices)) {
        try {
          const result = await deviceManager.controlDevice(device.id, {});
          if (result.success) synced++;
        } catch {
          // 忽略失败
        }
      }

      res.json({ success: true, synced });
    }
  };
}
