# 手咪 Claw - AI 助手

> 运行在 ESP32-S3 上的语音控制 AI 助手，配备 1.85 英寸圆形显示屏。
## 功能特性

### 本体

- **主控芯片**: ESP32-S3 
- **显示屏**: 1.85 英寸 ，360×360 圆形
- **麦克风**: I2S MEMS 麦克风 
- **存储**: 16MB Flash + SD 卡

### 🎙️ 语音 AI

- **ASR 集成**: 通过 WebSocket 实时语音识别
- **I2S 麦克风**: 高品质音频采集 (PCM)
- **离线识别**: 通过 ASR 服务器进行本地音频处理
- **显示**: 识别文字显示在圆形屏幕中央

### 💬 LLM 聊天

- **多提供商支持**: MiniMax、OpenAI、Anthropic、Ollama
- **流式响应**: 实时文字显示
- **Telegram 机器人**: 通过 Telegram 远程控制
- **上下文记忆**: 基于会话的对话历史

### ⏰ 自动化

- **定时任务**: 计划任务执行
- **心跳**: 定期状态通知
- **消息总线**: 组件间通信

### 🌐 连接

- **WiFi**: 内置 WiFi 管理器
- **WebSocket 服务器**: 本地控制界面 (端口 18789)
- **HTTP 代理**: 远程 API 访问
- **OTA 更新**: 无线固件更新

### 🎨 界面

- **圆形 LCD**: 1.85 英寸 圆形显示屏
- **"Hand Mi" 品牌**: 语音 AI 身份标识
- **聊天界面**: 消息历史显示
- **状态指示器**: WiFi、消息、运行时间

## 快速开始

### 1. 配置

编辑 `main/mimi_secrets.h`:
```c
#define MIMI_SECRET_WIFI_SSID "您的WiFi名称"
#define MIMI_SECRET_WIFI_PASSWORD "您的WiFi密码"
#define MIMI_SECRET_TELEGRAM_BOT_TOKEN "您的机器人Token"
#define MIMI_SECRET_MINIMAX_API_KEY "您的API密钥"
#define MIMI_SECRET_FUNASR_HOST "192.168.1.100"
#define MIMI_SECRET_FUNASR_PORT 10095
```

### 2. 编译和烧录

```bash
source $HOME/.espressif/esp-idf-v5.5.2/export.sh
idf.py build
idf.py flash monitor
```

### 3. FunASR 服务器 (可选)

运行语音识别需要 FunASR WebSocket 服务器:
```bash
python funasr_wss_server.py --port 10095
```

## 模块概览

| 模块 | 描述 |
|------|------|
| `audio/` | I2S 麦克风、WebSocket 客户端、FunASR 流式传输 |
| `llm/` | LLM 代理 (MiniMax、OpenAI、Anthropic、Ollama) |
| `telegram/` | Telegram 机器人集成 |
| `ui/` | LVGL 显示、聊天界面、字幕叠加 |
| `wifi/` | WiFi 管理器 |
| `gateway/` | 本地控制的 WebSocket 服务器 |
| `cron/` | 计划任务执行 |
| `heartbeat/` | 定期状态报告 |
| `tools/` | 网页搜索、时间、文件、定时工具 |
| `memory/` | 会话和持久化存储 |

## 架构

```
┌─────────────────────────────────────────┐
│            ESP32-S3 硬件                    │
├─────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────────────────┐  │
│  │   I2S   │  │    1.85" LCD       │  │
│  │   麦克风 │  │    (360×360)       │  │
│  └────┬─────┘  └─────────┬──────────┘  │
│       │                    │             │
│  ┌────▼────────────────────▼──────────┐  │
│  │         音频流 (WebSocket)          │  │
│  └─────────────┬──────────────────────┘  │
│                │ FunASR 服务器            │
│  ┌────────────▼──────────────────────┐  │
│  │         LLM 聊天流程                 │  │
│  │  (MiniMax/OpenAI/Anthropic)       │  │
│  └─────────────┬──────────────────────┘  │
│                │                         │
│  ┌────────────▼──────────────────────┐  │
│  │       WebSocket 服务器              │  │
│  │    (端口 18789)                   │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.1 | 2026-03-07 | FunASR 语音流、字幕显示 |
| v1.0.0 | 2026-03-07 | 初始版本，支持 LLM 聊天 |

## 许可证

MIT 许可证
