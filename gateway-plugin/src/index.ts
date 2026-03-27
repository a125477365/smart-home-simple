/**
 * OpenClaw 智能家居插件入口
 * 提供设备管理工具和 Gateway Web 界面
 */

import { definePluginEntry } from 'openclaw/plugin-sdk/plugin-entry';
import { Type } from '@sinclair/typebox';
import type { OpenClawPluginApi } from 'openclaw/plugin-sdk/types';
import { DeviceManager } from './device-manager.js';

export interface SmartHomeConfig {
  apiPort?: number;
  autoDiscover?: boolean;
  discoverInterval?: number;
}

export default definePluginEntry({
  id: 'smart-home',
  name: 'Smart Home Control',
  description: '智能家居控制面板 - 管理局域网 ESP32 设备',

  configSchema: {
    type: 'object',
    additionalProperties: false,
    properties: {
      apiPort: { type: 'number', default: 43210 },
      autoDiscover: { type: 'boolean', default: true },
      discoverInterval: { type: 'number', default: 60000 }
    }
  },

  async register(api: OpenClawPluginApi) {
    const config = api.config as SmartHomeConfig;
    const deviceManager = new DeviceManager(config);

    // ========== 注册 Agent 工具 ==========

    api.registerTool({
      name: 'smart_home_list_devices',
      description: '列出所有已配置的智能家居设备',
      parameters: Type.Object({}),
      async execute() {
        const devices = await deviceManager.listDevices();
        return {
          content: [{
            type: 'text',
            text: JSON.stringify(devices, null, 2)
          }]
        };
      }
    });

    api.registerTool({
      name: 'smart_home_control',
      description: '控制智能家居设备（开关、亮度）',
      parameters: Type.Object({
        device_id: Type.String({ description: '设备ID或名称' }),
        state: Type.Optional(Type.Boolean({ description: '开关状态' })),
        brightness: Type.Optional(Type.Number({ description: '亮度 0-100' }))
      }),
      async execute(_id, params) {
        const result = await deviceManager.controlDevice(
          params.device_id,
          { state: params.state, brightness: params.brightness }
        );
        return {
          content: [{
            type: 'text',
            text: result.success
              ? `设备 ${params.device_id} 已更新`
              : `操作失败: ${result.error}`
          }]
        };
      }
    });

    api.registerTool({
      name: 'smart_home_discover',
      description: '扫描局域网发现新的智能家居设备',
      parameters: Type.Object({
        timeout: Type.Optional(Type.Number({ description: '扫描超时秒数', default: 5 }))
      }),
      async execute(_id, params) {
        const devices = await deviceManager.discoverDevices(params.timeout || 5);
        return {
          content: [{
            type: 'text',
            text: `发现 ${devices.length} 个设备:\n${JSON.stringify(devices, null, 2)}`
          }]
        };
      }
    });

    api.registerTool({
      name: 'smart_home_all_on',
      description: '打开所有智能家居设备',
      parameters: Type.Object({}),
      async execute() {
        const result = await deviceManager.controlAll({ state: true });
        return {
          content: [{
            type: 'text',
            text: `已打开 ${result.success}/${result.total} 个设备`
          }]
        };
      }
    });

    api.registerTool({
      name: 'smart_home_all_off',
      description: '关闭所有智能家居设备',
      parameters: Type.Object({}),
      async execute() {
        const result = await deviceManager.controlAll({ state: false });
        return {
          content: [{
            type: 'text',
            text: `已关闭 ${result.success}/${result.total} 个设备`
          }]
        };
      }
    });

    // ========== 注册 Gateway HTTP 路由 ==========

    // 认证中间件 - 检查请求是否来自已认证的 Gateway 会话
    const requireAuth = (handler: Function) => {
      return async (req: any, res: any) => {
        // 检查来源是否为 Gateway 本地
        const origin = req.headers?.origin || '';
        const referer = req.headers?.referer || '';
        const host = req.headers?.host || 'localhost:18789';

        // 允许来自 Gateway Control UI 的请求
        const isFromGateway = origin.includes(host) ||
                              referer.includes(host) ||
                              req.ip === '127.0.0.1' ||
                              req.ip === '::1';

        if (!isFromGateway) {
          // 检查 Authorization header
          const authHeader = req.headers?.authorization || '';
          const token = authHeader.replace('Bearer ', '');

          // 验证 token（与 Gateway 共享配置）
          const gatewayToken = process.env.OPENCLAW_GATEWAY_TOKEN;
          if (!gatewayToken || token !== gatewayToken) {
            return res.status(401).json({ error: 'Unauthorized' });
          }
        }

        return handler(req, res);
      };
    };

    // 控制面板页面（无需认证，前端会检查）
    api.registerGatewayMethod?.({
      method: 'GET',
      path: '/smarthome',
      handler: async (req, res) => {
        res.setHeader('Content-Type', 'text/html; charset=utf-8');
        res.send(await deviceManager.getWebPage());
      }
    });

    // API: 获取设备列表（需要认证）
    api.registerGatewayMethod?.({
      method: 'GET',
      path: '/api/smarthome/devices',
      handler: requireAuth(async (req: any, res: any) => {
        res.json(await deviceManager.listDevices());
      })
    });

    // API: 发现新设备（需要认证）
    api.registerGatewayMethod?.({
      method: 'POST',
      path: '/api/smarthome/discover',
      handler: requireAuth(async (req: any, res: any) => {
        const devices = await deviceManager.discoverDevices(5);
        res.json({ devices, count: devices.length });
      })
    });

    // API: 同步设备状态（需要认证）
    api.registerGatewayMethod?.({
      method: 'POST',
      path: '/api/smarthome/sync',
      handler: requireAuth(async (req: any, res: any) => {
        const result = await deviceManager.syncAllDevices();
        res.json(result);
      })
    });

    // API: 控制单个设备（需要认证）
    api.registerGatewayMethod?.({
      method: 'POST',
      path: '/api/smarthome/control/:deviceId',
      handler: requireAuth(async (req: any, res: any) => {
        const result = await deviceManager.controlDevice(
          req.params.deviceId,
          req.body
        );
        res.json(result);
      })
    });

    // API: 控制所有设备（需要认证）
    api.registerGatewayMethod?.({
      method: 'POST',
      path: '/api/smarthome/control/all',
      handler: requireAuth(async (req: any, res: any) => {
        const result = await deviceManager.controlAll(req.body);
        res.json(result);
      })
    });

    // API: 添加设备（需要认证）
    api.registerGatewayMethod?.({
      method: 'POST',
      path: '/api/smarthome/devices',
      handler: requireAuth(async (req: any, res: any) => {
        const device = await deviceManager.addDevice(req.body);
        res.json({ success: true, device });
      })
    });

    // API: 移除设备（需要认证）
    api.registerGatewayMethod?.({
      method: 'DELETE',
      path: '/api/smarthome/devices/:deviceId',
      handler: requireAuth(async (req: any, res: any) => {
        await deviceManager.removeDevice(req.params.deviceId);
        res.json({ success: true });
      })
    });

    // ========== 注册 Control UI 导航链接 ==========
    // 在 Gateway Control UI 中添加 "智能家居" 入口
    api.registerNavLink?.({
      id: 'smart-home',
      label: '智能家居',
      icon: 'home',
      path: '/smarthome',
      order: 100  // 排在设置之后
    });

    // 启动自动发现
    if (config.autoDiscover) {
      deviceManager.startAutoDiscover(config.discoverInterval || 60000);
    }

    console.log('[SmartHome] Plugin registered');
    console.log('[SmartHome] Control panel: http://localhost:18789/smarthome');
  }
});
