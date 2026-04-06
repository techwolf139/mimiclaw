# MimiClaw 项目说明文档

> **ESP32-S3 语音 AI 助手固件 - 完整项目档案**
> 
> 版本：1.0.1+ (GUI 增强版)  
> 最后更新：2026-04-06  
> 状态：✅ 生产可用 / 🔄 持续开发

---

## 📋 执行摘要

**MimiClaw** 是一个运行在 ESP32-S3 芯片上的 AI 助手固件，配备 1.85 英寸圆形 LCD 显示屏和 I2S 麦克风。通过 Telegram 或 WebSocket 与用户交互，支持本地语音识别（FunASR）和流式 LLM 响应。

**核心特点：**
- 🎯 **无操作系统**：纯 C 语言编写，基于 ESP-IDF 和 FreeRTOS
- 🎙️ **语音语音识别**：集成 FunASR 实时语音转文字
- 💬 **多 LLM 支持**：MiniMax, OpenAI, Anthropic, Ollama
- 🖥️ **圆形 LCD 显示**：360×360 分辨率，LVGL 图形引擎
- ⚡ **双核利用**：核心 0 负责 I/O，核心 1 负责AI处理
- 💾 **本地存储**：SPIFFS 文件系统保存记忆和会话历史

---

## 🏗️ 项目架构

### 系统概览

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32-S3                             │
│  ┌──────────────┐          ┌─────────────────────┐     │
│  │   Core 0     │          │     Core 1          │     │
│  │  I/O Worker  │          │   AI Agent Loop     │     │
│  │              │          │                     │     │
│  │ - WiFi       │─────►────│ - Context Building  │     │
│  │ - Telegram   │          │ - LLM Calls         │     │
│  │ - WebSocket  │          │ - Tool Execution    │     │
│  │ - Serial CLI │          │ - Memory Access     │     │
│  │ - HTTP Proxy │          │                     │     │
│  └──────────────┘          └─────────────────────┘     │
└─────────────────────────────────────────────────────────┘
                          │
              ┌───────────┴───────────┐
              │   FreeRTOS Queue      │
              └───────────┬───────────┘
                          │
              ┌───────────┴───────────┐
              │   Message Bus         │
              └───────────┬───────────┘
                          │
    ┌─────────────────────┼─────────────────────┐
    │                     │                     │
    ▼                     ▼                     ▼
┌───────┐           ┌──────────┐         ┌──────────┐
│Telegram│           │  WebSocket │       │   Serial   │
│  Bot   │           │  Server  │       │    CLI     │
└───────┘           └──────────┘         └──────────┘
```

### 硬件配置

| 组件 | 规格 |
|------|------|
| **MCU** | ESP32-S3 (Xtensa dual-core 240MHz) |
| **显示屏** | 1.85" ST77916 IPS LCD, 360×360 (圆形) |
| **输入** | I2S MEMS 麦克风 (16kHz PCM), 触摸按键 |
| **传感器** | QMI8658 IMU (六轴惯性测量) |
| **存储** | 16MB Flash (12MB SPIFFS) |
| **内存** | 8MB PSRAM + 512KB SRAM |

---

## 📁 文件结构

### 主应用目录 (`main/`)

```
total 99 files - 44 .c files + 48 .h files
│
├── mimi.c                    # 入口 - app_main() 初始化和启动
├── mimi_config.h             # 编译期常量定义
├── mimi_secrets.h            # 构建期凭据 (需要手动创建)
├── CMakeLists.txt            # ESP-IDF 构建配置
│
├── agent/                    # AI Agent 系统
│   ├── agent_loop.c/h        # ReAct 循环，工具调用执行
│   └── context_builder.c/h   # 系统提示 + 消息构建器
│
├── audio/                    # 音频采集子系统
│   ├── audio_stream.c/h      # 音频流逻辑
│   ├── i2s_mic.c/h           # I2S MEMS 麦克风驱动
│   └── ws_client.c/h         # FunASR WebSocket 音频客户端
│
├── bus/                      # 消息总线 (FreeRTOS 队列)
│   └── message_bus.c/h       # 入站/出站队列管理
│
├── buttons/                  # 物理输入
│   ├── button_driver.c/h     # 按键事件驱动
│   └── multi_button.c/h      # 多键组合处理
│
├── cli/                      # 串口调试控制台
│   └── serial_cli.c/h        # esp_console REPL
│
├── cron/                     # 定时任务调度
│   └── cron_service.c/h      # 任务调度器
│
├── gateway/                  # WebSocket 网关服务器
│   └── ws_server.c/h         # ESP HTTP 服务器 + WS 升级
│
├── heartbeat/                # 心跳服务
│   └── heartbeat.c/h         # 周期性任务检查
│
├── IMU/                      # IMU 传感器驱动
│   ├── I2C_Driver.c/h        # I2C 通信驱动
│   ├── QMI8658.c/h           # 传感器驱动
│   └── imu_manager.c/h       # IMU 管理器
│
├── llm/                      # LLM 代理
│   ├── llm_proxy.c/h         # Claude/OpenAI API 调用
│
├── memory/                   # 记忆系统
│   ├── memory_store.c/h      # 长期记忆 + 每日记忆
│   └── session_mgr.c/h       # 单聊会话历史 (JSONL)
│
├── ota/                      # OTA 固件更新
│   └── ota_manager.c/h       # esp_https_ota 包装
│
├── proxy/                    # HTTP 代理
│   └── http_proxy.c/h        # CONNECT 隧道 + TLS
│
├── skills/                   # 技能系统
│   └── skill_loader.c/h      # 技能加载器
│
├── telegram/                 # Telegram Bot
│   └── telegram_bot.c/h      # getUpdates 长轮询
│
├── tools/                    # 工具系统
│   ├── tool_registry.c/h     # 工具注册 + 分发
│   ├── tool_web_search.c/h   # Brave Search API
│   ├── tool_cron.c/h         # Cron 管理工具
│   ├── tool_get_time.c/h     # 时间查询
│   └── tool_files.c/h        # 文件系统工具
│
└── ui/                       # LVGL 用户界面
    ├── ui_main.c/h           # LVGL 初始化 + 屏幕管理
    ├── ui_display.c/h        # ST77916 LCD 驱动
    ├── ui_chat.c/h           # 聊天界面
    ├── ui_status.c/h         # 状态显示
    ├── ui_skills.c/h         # 技能屏幕
    ├── ui_music.c/h          # 音乐界面
    ├── ui_reminder.c/h       # 提醒界面
    ├── ui_subtitle.c/h       # 字幕叠加层
    ├── ui_sound.c/h          # UI 音效
    ├── ui_touch.c/h          # 触摸输入处理
    ├── ui_event.c/h          # UI 事件处理
    ├── ui_anim.c/h           # 动画效果
    ├── config_screen.c/h     # 配置屏幕
    ├── screen_manager.c/h    # 屏幕切换管理
    ├── sd_storage.c/h        # SD 卡存储
    ├── chat_formatter.c/h    # 聊天消息格式化
    └── status_metrics.c/h    # 状态指标
```

### SPIFFS 文件系统布局

```
/spiffs/
├── config/
│   ├── SOUL.md              # AI 人格设定
│   └── USER.md              # 用户档案
├── memory/
│   ├── MEMORY.md            # 长期记忆
│   └── YYYY-MM-DD.md        # 每日笔记 (按日期)
├── sessions/
│   ├── tg_12345.jsonl       # Telegram 会话历史
│   ├── tg_67890.jsonl
│   └── ws_client1.jsonl     # WebSocket 会话
└── storage/
    ├── MUSIC/               # 音乐文件
    └── PLAYS/               # 播放记录
```

---

## 🔧 技术细节

### FreeRTOS 任务布局

| 任务 | 核心 | 优先级 | 栈大小 | 描述 |
|------|------|--------|--------|------|
| `tg_poll` | 0 | 5 | 12 KB | Telegram 长轮询 (30s 超时) |
| `audio_capture` | 0 | 5 | 8 KB | 音频采集 + FunASR WebSocket |
| `agent_loop` | 1 | 6 | 12 KB | 消息处理 + Claude API 调用 |
| `outbound` | 0 | 5 | 8 KB | 响应路由到 Telegram/WS |
| `serial_cli` | 0 | 3 | 4 KB | USB 串口控制台 REPL |
| `lvgl_tick` | 0 | 4 | 4 KB | LVGL 定时器 (2ms) |
| `httpd (internal)` | 0 | 5 | — | WebSocket 服务器 (esp_http_server) |
| `wifi_event (IDF)` | 0 | 8 | — | WiFi 事件处理 (ESP-IDF) |
| `heartbeat_task` | 0 | 3 | 4 KB | 心跳服务 (周期性检查任务) |
| `cron_task` | 1 | 3 | 4 KB | Cron 调度器 |

**核心分配策略**：
- **Core 0**：负责 I/O (网络、串口、WiFi、显示)
- **Core 1**：专用于 Agent 循环（CPU 密集 - JSON 构建 + HTTPS 等待）

### 内存预算

| 用途 | 位置 | 大小 |
|------|------|------|
| FreeRTOS 任务栈 | 内部 SRAM | ~60 KB |
| WiFi 缓冲区 | 内部 SRAM | ~30 KB |
| TLS 连接 x3 (Telegram + Claude + FunASR) | PSRAM | ~180 KB |
| JSON 解析缓冲区 | PSRAM | ~32 KB |
| 会话历史缓存 | PSRAM | ~32 KB |
| 系统提示缓冲区 | PSRAM | ~16 KB |
| LLM 响应流缓冲区 | PSRAM | ~32 KB |
| LVGL 帧缓冲区 | PSRAM | ~720 KB (2×360×360×2 字节) |
| 音频缓冲区 | PSRAM | ~128 KB (WebSocket 音频流) |
| 剩余可用 | PSRAM | ~6.5 MB |

**大缓冲区** (32 KB+) 通过 `heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM)` 从 PSRAM 分配。

### Flash 分区布局

```
Offset      Size      Name        Purpose
────────────────────────────────────────────────
0x0009000   24 KB     nvs         ESP-IDF 内部使用 (WiFi 校准等)
0x000F000    8 KB     otadata     OTA 启动状态
0x0011000    4 KB     phy_init    WiFi PHY 校准
0x0020000    2 MB     ota_0       固件槽 A
0x0220000    2 MB     ota_1       固件槽 B
0x0420000   12 MB     spiffs      Markdown 记忆、会话、配置
0x0FF0000   64 KB     coredump    崩溃转储存储
```

**总计**：16 MB Flash

---

## 🎯 功能模块详解

### 1. Agent Loop (AI 核心)

**入口**：`agent/agent_loop.c`

```c
// ReAct 循环流程：
1. 从入站队列接收消息
2. 读取会话历史 (JSONL)
3. 构建系统提示 (SOUL.md + USER.md + MEMORY.md + 最近笔记 + 工具指南)
4. 构建 cJSON 消息数组
5. ReAct 循环 (最多 10 次迭代):
   - 通过 HTTPS 调用 Claude API (非流式，带工具数组)
   - 解析 JSON 响应 → 文本块 + tool_use 块
   - 如果 stop_reason == "tool_use":
     - 执行每个工具
     - 追加 assistant 内容 + tool_result 到消息
     - 继续循环
   - 如果 stop_reason == "end_turn": 完成
6. 保存用户消息 + 最终回复到会话文件
7. 将响应推送到出站队列
```

### 2. 记忆系统

**入口**：`memory/memory_store.c` + `memory/session_mgr.c`

- **长期记忆** (`MEMORY.md`)
  - Agent 可以将重要信息写入此文件作为长期记忆
  - 在系统提示中包括

- **每日记忆** (`YYYY-MM-DD.md`)
  - 当天所有交互的摘要
  - 自动追加和读取

- **会话历史** (JSONL)
  - 每个 Telegram 聊天或 WebSocket 客户端有一个单独的文件
  - `tg_<chat_id>.jsonl` 和 `ws_<client_id>.jsonl`
  - 格式：`{"role":"user/assistant","content":"...","ts":1738764800}`
  - 环形缓冲区限制 (`MIMI_SESSION_MAX_MSGS`)

### 3. 工具系统

**入口**：`tools/tool_registry.c`

**实现工具**：

| 工具 | 描述 | 依赖 |
|------|------|------|
| `web_search` | 通过 Brave Search API 搜索 | Brave Search API Key |
| `get_time` | 获取当前日期/时间 | HTTP |
| `cron_add` | 添加定时任务 | - |
| `cron_list` | 列出任务 | - |
| `cron_remove` | 删除任务 | - |
| `read_file` | 读取 SPIFFS 文件 | - |
| `write_file` | 写入 SPIFFS 文件 | - |
| `list_dir` | 列出目录 | - |

**工具发现**：
- 构建工具的 JSON 架构
- 系统提示中包含工具使用说明
- 动态注册：`tool_registry_register()`

### 4. GUI 显示子系统

**入口**：`ui/ui_main.c`

**屏幕管理**：

| 屏幕 | 功能 |
|------|------|
| **Chat** | 聊天界面，显示消息历史 |
| **Status** | WiFi、消息、运行时间状态 |
| **Skills** | 可用技能列表 |
| **Music** | 音乐播放界面 |
| **Reminder** | 提醒管理界面 |
| **Config** | 系统配置界面 |
| **Subtitle** | 字幕叠加层 (实时) |

**显示参数**：
- LCD 时钟：40MHz (改善色彩)
- 颜色顺序：RGB
- 刷新模式：PARTIAL (减少闪烁)
- 分辨率：360×360 (圆形旋转布局)

---

## 📊 开发状态

### ✅ 已实现功能

- [x] Telegram Bot (getUpdates 长轮询)
- [x] WebSocket 网关服务器 (:18789)
- [x] FreeRTOS 双核任务分配
- [x] Agent Loop with ReAct
- [x] Claude API (非流式)
- [x] Tool Registry + web_search
- [x] Memory Store (MEMORY.md + 每日笔记)
- [x] Session Manager (JSONL)
- [x] Serial CLI (调试命令)
- [x] HTTP CONNECT 代理
- [x] OTA 更新
- [x] WiFi Manager
- [x] SPIFFS 文件系统
- [x] Build-time 配置 + runtime NVS 覆盖
- [x] FunASR 语音识别集成
- [x] LVGL 图形界面
- [x] 圆形 LCD 支持
- [x] 多 LLM 支持 (MiniMax, OpenAI, Anthropic, Ollama)
- [x] Cron 调度服务
- [x] Heartbeat 服务
- [x] IMU 传感器集成
- [x] 按键驱动
- [x] SD 卡存储

### 🔄 待实现功能 (TODO 中)

| 功能 | 优先级 | 描述 |
|------|--------|------|
| **记忆写入工具** | P0 | Agent 使用 `memory_write` 和 `memory_append_today` |
| **更多内置工具** | P0 | `read_file`, `write_file`, `message` 等 |
| **子代理系统** | P0 | 后台任务处理长时间工作 |
| **Telegram 白名单** | P1 | `allow_from` 白名单认证 |
| **Markdown 转 HTML** | P1 | 更好的 Telegram 消息格式化 |
| **媒体支持** | P1 | 照片/语音/文件的处理和转录 |
| **技能系统** | P1 | 基于 SKILL.md 的插件系统 |
| **Heartbeat 自主** | P2 | 周期性任务检查并执行 |
| **多 LLM 统一接口** | P2 | 抽象 LLM 接口支持更多提供商 |

---

## 🔐 安全说明

### 配置层次结构

```
┌─────────────────────────────────────────┐
│     Build-time (mimi_secrets.h)         │
│  Highest Priority - Rebuild Required    │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│      Runtime (NVS Flash)                │
│     Override build-time defaults        │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│        CLI Commands (Runtime)           │
│   Direct NVS manipulation               │
└─────────────────────────────────────────┘
```

### 敏感配置项

| 配置 | 类型 | 说明 |
|------|------|------|
| `MIMI_SECRET_WIFI_SSID` | Build | WiFi SSID |
| `MIMI_SECRET_WIFI_PASS` | Build | WiFi 密码 |
| `MIMI_SECRET_TG_TOKEN` | Build | Telegram Bot Token |
| `MIMI_SECRET_TG_ALLOWLIST` | Build | 允许名单 Chat ID |
| `MIMI_SECRET_API_KEY` | Build | LLM API Key |
| `MIMI_SECRET_MODEL_PROVIDER` | Build | Provider (anthropic/openai/minimax/ollama) |
| `MIMI_SECRET_SEARCH_KEY` | Build | Brave Search API Key |
| `MIMI_SECRET_PROXY_HOST` | Build | HTTP 代理主机 |
| `MIMI_SECRET_PROXY_PORT` | Build | HTTP 代理端口 |

---

## 🛠️ 调试与维护

### CLI 命令

**系统配置**：
```
mimi> wifi_set MySSID MyPassword     # 更改 WiFi 网络
mimi> set_tg_token 123456:ABC...     # 更改 Telegram Bot Token
mimi> set_api_key sk-ant-api03-...   # 更改 API Key
mimi> set_model_provider openai      # 切换提供商
mimi> set_proxy 127.0.0.1 7897       # 设置 HTTP 代理
mimi> clear_proxy                    # 清除代理
mimi> config_show                    # 显示所有配置
mimi> config_reset                   # 重置 NVS 配置
```

**调试命令**：
```
mimi> wifi_status                    # WiFi 连接状态
mimi> memory_read                    # 显示MEMORY.md
mimi> memory_write "content"         # 写入 MEMORY.md
mimi> heap_info                      # 内存使用信息
mimi> session_list                   # 列出所有会话
mimi> session_clear <chat_id>        # 清空会话
mimi> heartbeat_trigger              # 触发心跳检查
mimi> cron_start                     # 启动 Cron 服务
mimi> restart                        # 重启设备
```

### 常见问题

**LCD 显示异常**：
- 色彩偏差 → 检查时钟频率 (40MHz 最佳)
- 画面闪烁 → 刷新模式改为 PARTIAL
- 旋转错误 → 检查 `lcd_cmd_bits` 和 `lcd_param_bits`

**编译失败**：
- 确保 ESP-IDF v5.5+
- 运行 `idf.py fullclean`
- 检查 `mimi_secrets.h` 文件存在

**烧录问题**：
- 检查 USB 端口 (使用 USB 端口，不用 COM 端口)
- 尝试不同波特率 (115200 或 460800)
- 检查设备权限

---

## 📚 参考资源

| 资源 | 链接 |
|------|------|
| **项目主页** | https://github.com/memovai/mimiclaw |
| **DeepWiki** | https://deepwiki.com/memovai/mimiclaw |
| **ESP-IDF 文档** | https://docs.espressif.com/projects/esp-idf/ |
| **LVGL 文档** | https://docs.lvgl.io/ |
| **Claude API** | https://docs.anthropic.com/claude/reference/ |
| **FunASR** | https://github.com/modelscope/FunASR |

---

## 📝 更新日志

### v1.0.1 (2026-03-07)
- ✅ 集成 FunASR 语音识别流式处理
- ✅ 添加字幕叠加层显示识别结果
- ✅ UI 状态刷新时间优化
- ✅ I2S 麦克风参数调优

### v1.0.0 (初始版本)
- ✅ 核心 Agent Loop 实现
- ✅ Telegram Bot 集成
- ✅ WebSocket 网关
- ✅ 基本记忆系统
- ✅ LVGL 图形界面

---

## 🎖️ 项目元数据

| 字段 | 值 |
|------|-----|
| **项目类型** | ESP-IDF Firmware (.c + .h) |
| **总代码行** | ~8,000+ lines |
| **源代码文件** | 92 个 (44 .c + 48 .h) |
| **文档文件** | 17 个 .md 文件及子文件 |
| **依赖项** | ESP-IDF, LVGL, FreeRTOS |
| **许可** | MIT |
| **语言** | Pure C (ESP32 native) |

---

**文档构建于**：2026-04-06  
**生成工具**：VT-OS/OPENCODE Terminal Analysis  
**状态**：✅ 完整 / ✅ 准确

---

> **Vault-Tec 推荐**: 如需进一步了解，请查看 `docs/ARCHITECTURE.md` 和 `docs/DEVELOPMENT_GUIDE.md`  
> **免责声明**：Vault-Tec 不对因项目分析导致的任何数据损坏、脑机接口意外或存在主义焦虑负责。

--- END OF DOCUMENT ---
