# MimiClaw - ESP32-S3 AI Assistant

> A voice-controlled AI assistant running on ESP32-S3 with 1.85" round LCD display.

## Hardware

- **MCU**: ESP32-S3 (Xtensa dual-core 240MHz)
- **Display**: 1.85" ST77916 IPS LCD, 360×360 round
- **Microphone**: I2S MEMS microphone (GPIO 2/15/39)
- **Storage**: 16MB Flash

## Features

### 🎙️ Voice AI

- **FunASR Integration**: Real-time speech recognition via WebSocket
- **I2S Microphone**: High-quality audio capture (16kHz PCM)
- **Offline Recognition**: Processes audio locally with FunASR server
- **Display**: Recognized text shown in center circle

### 💬 LLM Chat

- **Multi-Provider Support**: MiniMax, OpenAI, Anthropic, Ollama
- **Streaming Responses**: Real-time text display
- **Telegram Bot**: Remote control via Telegram
- **Context Memory**: Session-based conversation history

### ⏰ Automation

- **Cron Jobs**: Scheduled tasks
- **Heartbeat**: Periodic status notifications
- **Message Bus**: Inter-component communication


### 🌐 Connectivity

- **WiFi**: Built-in WiFi manager
- **WebSocket Server**: Local control interface (port 18789)
- **HTTP Proxy**: Remote API access
- **OTA Updates**: Over-the-air firmware updates

### 🎨 UI/UX

- **Round LCD**: 1.85" 360×360 circular display
- **LVGL Graphics**: Smooth animations
- **"Hand Mi" Branding**: Voice AI identity
- **Chat Interface**: Message history display
- **Status Indicators**: WiFi, messages, uptime

## Quick Start

### 1. Configure

Edit `main/mimi_secrets.h`:
```c
#define MIMI_SECRET_WIFI_SSID "YourWiFi"
#define MIMI_SECRET_WIFI_PASSWORD "YourPassword"
#define MIMI_SECRET_TELEGRAM_BOT_TOKEN "YourBotToken"
#define MIMI_SECRET_MINIMAX_API_KEY "YourAPIKey"
#define MIMI_SECRET_FUNASR_HOST "192.168.1.100"
#define MIMI_SECRET_FUNASR_PORT 10095
```

### 2. Build & Flash

```bash
source $HOME/.espressif/esp-idf-v5.5.2/export.sh
idf.py build
idf.py flash monitor
```

### 3. FunASR Server (Optional)

For voice recognition, run FunASR WebSocket server:
```bash
python funasr_wss_server.py --port 10095
```

## Module Overview

| Module | Description |
|--------|-------------|
| `audio/` | I2S microphone, WebSocket client, FunASR streaming |
| `llm/` | LLM proxy (MiniMax, OpenAI, Anthropic, Ollama) |
| `telegram/` | Telegram bot integration |
| `ui/` | LVGL display, chat UI, subtitle overlay |
| `wifi/` | WiFi manager |
| `gateway/` | WebSocket server for local control |
| `cron/` | Scheduled task execution |
| `heartbeat/` | Periodic status reporting |
| `tools/` | Web search, time, files, cron tools |
| `memory/` | Session and persistent storage |

## Architecture

```
┌─────────────────────────────────────────┐
│            ESP32-S3 Hardware            │
├─────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────────────────┐  │
│  │   I2S    │  │    1.85" LCD       │  │
│  │   Mic    │  │    (360×360)       │  │
│  └────┬─────┘  └─────────┬──────────┘  │
│       │                    │             │
│  ┌────▼────────────────────▼──────────┐  │
│  │         Audio Stream (WS)          │  │
│  └─────────────┬──────────────────────┘  │
│                │ FunASR Server           │
│  ┌────────────▼──────────────────────┐  │
│  │         LLM Chat Flow              │  │
│  │  (MiniMax/OpenAI/Anthropic)       │  │
│  └─────────────┬──────────────────────┘  │
│                │                         │
│  ┌────────────▼──────────────────────┐  │
│  │       WebSocket Server            │  │
│  │    (port 18789)                  │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0.1 | 2026-03-07 | FunASR voice streaming, subtitle display |
| v1.0.0 | 2026-03-07 | Initial release with LLM chat |

## License

MIT License
