# MimiClaw 开发指南

> ESP32-S3 AI 助手固件 - C/FreeRTOS 裸机实现开发文档  
> **版本**: 2.0.0  
> **最后更新**: 2026-04-06  
> **维护状态**: ✅ 活跃维护

---

## 文档索引

本文档是 MimiClaw 项目的主要开发参考。所有相关文档链接请参见下方的 [参考资料](#参考资料) 和 [项目文档索引](#项目文档索引) 章节。

### 📚 完整文档导航

| 类型 | 文档 | 说明 |
|------|------|------|
| **核心文档** | [📄 PROJECT_SUMMARY.md](./PROJECT_SUMMARY.md) | 项目完整说明和元数据 |
| **架构图集** | [📊 ARCHITECTURE_DIAGRAMS.md](./ARCHITECTURE_DIAGRAMS.md) | 6 个可视化架构图 |
| **系统架构** | [📄 ARCHITECTURE.md](./ARCHITECTURE.md) | 详细系统架构设计 |
| **功能清单** | [📄 FEATURES.md](./FEATURES.md) | 当前功能介绍 |
| **路线图** | [📄 TODO.md](./TODO.md) | 待实现功能追踪 |
| **UI/UX 设计** | [📝 docs/plans/](./plans/) | 多个设计文档 |

---


## 项目概述

MimiClaw 是一款运行在 ESP32-S3 芯片上的 AI 助手，通过 Telegram 或 WebSocket 与用户交互，支持本地记忆和工具调用。

### 核心特性

- **无操作系统**: 纯 C 语言编写，基于 ESP-IDF 和 FreeRTOS
- **双核分配**: 核心 0 负责 I/O，核心 1 负责 AI 处理
- **本地存储**: SPIFFS 文件系统保存记忆和会话历史
- **多通道**: Telegram Bot API 和 WebSocket 服务器 (18789 端口)
- **可扩展性**: 工具系统支持通过 Brave Search API 进行联网搜索

---

## 快速开始

### 环境要求

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# macOS
xcode-select --install
brew install cmake ninja python3
```

### ESP-IDF 安装

```bash
# 安装 ESP-IDF v5.5+
git clone --recursive https://github.com/espressif/esp-idf.git --depth 1 --branch v5.5.2
./esp-idf/install.sh
./esp-idf/export.sh
```

### 构建流程

```bash
# 克隆项目
git clone https://github.com/memovai/mimiclaw.git
cd mimiclaw

# 设置目标
idf.py set-target esp32s3

# 修改配置文件（需要全清）
cp main/mimi_secrets.h.example main/mimi_secrets.h
# 编辑 main/mimi_secrets.h 填入你的配置

# 清洁构建（修改 secrets 后必须执行）
idf.py fullclean

# 编译
idf.py build

# 烧录并监控
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 项目结构

```
mimiclaw/
├── main/                         # ESP-IDF 主应用目录
│   ├── mimi.c                   # 入口文件 - app_main() 协调初始化
│   ├── mimi_config.h            # 编译期常量定义
│   ├── mimi_secrets.h           # 构建期凭据（需要手动创建）
│   ├── CMakeLists.txt           # ESP-IDF 构建配置
│   ├── idf_component.yml        # ESP-IDF 组件描述
│   │
│   ├── bus/                     # 消息总线
│   │   ├── message_bus.h        # mimi_msg_t 结构体，队列 API
│   │   └── message_bus.c        # 双向 FreeRTOS 队列：入队/出队
│   │
│   ├── wifi/                    # WiFi 管理器
│   │   ├── wifi_manager.h       # WiFi STA 生命周期 API
│   │   └── wifi_manager.c       # 事件处理，指数退避重连
│   │
│   ├── telegram/                # Telegram Bot
│   │   ├── telegram_bot.h       # Bot 初始化/启动，sendMessage API
│   │   └── telegram_bot.c       # 长轮询循环，JSON 解析，消息分片
│   │
│   ├── llm/                     # LLM 代理
│   │   ├── llm_proxy.h          # llm_chat() 和 llm_chat_tools() API
│   │   └── llm_proxy.c          # Anthropic Messages API（非流式）
│   │
│   ├── agent/                   # AI Agent
│   │   ├── agent_loop.h         # Agent 任务初始化/启动
│   │   ├── agent_loop.c         # ReAct 循环：LLM 调用→工具执行→重复
│   │   ├── context_builder.h    # 系统提示 + 消息构建器 API
│   │   └── context_builder.c    # 读取 bootstrap 文件 + 记忆 + 工具引导
│   │
│   ├── tools/                   # 工具系统
│   │   ├── tool_registry.h      # 工具定义、注册/分发 API
│   │   ├── tool_registry.c      # 工具注册，JSON 架构构建，按名分发
│   │   ├── tool_web_search.h    # 网络搜索工具 API
│   │   └── tool_web_search.c    # Brave Search API（直连 + 代理）
│   │
│   ├── memory/                  # 记忆系统
│   │   ├── memory_store.h       # 长期记忆和每日记忆 API
│   │   ├── memory_store.c       # MEMORY.md 读写，每日 .md 追加/读取
│   │   ├── session_mgr.h        # 单聊会话 API
│   │   └── session_mgr.c        # JSONL 会话文件，环形缓冲区历史
│   │
│   ├── gui/                     # GUI 子系统（圆屏显示）
│   │   ├── ui/
│   │   │   ├── ui_main.c        # 主 UI 宿主：LVGL 初始化，屏幕管理
│   │   │   ├── ui_main.h        # 主 UI 公共 API：ui_init, ui_start
│   │   │   ├── ui_display.c     # 显示驱动：ST77916 LCD 初始化与刷新
│   │   │   ├── ui_display.h     # 显示公共接口
│   │   │   ├── ui_chat.c/h      # 聊天 UI 渲染
│   │   │   ├── ui_status.c/h    # 状态屏幕
│   │   │   ├── ui_music.c/h     # 音乐屏幕
│   │   │   ├── ui_reminder.c/h  # 提醒屏幕
│   │   │   ├── ui_subtitle.c/h  # 字幕叠加层
│   │   │   ├── ui_sound.c/h     # UI 音效（蜂鸣器）
│   │   │   ├── ui_touch.c/h     # 触摸输入处理（CST816S I2C）
│   │   │   ├── ui_state.c/h     # 全局 UI 状态管理
│   │   │   ├── ui_skills.c/h    # 技能屏幕
│   │   │   ├── config_screen.c/h # 配置/设置屏幕控制器
│   │   │   ├── sd_storage.c/h   # SD 卡存储子系统
│   │   │   └── config_screen.h  # 配置屏幕 API
│   │   │
│   ├── gateway/                 # WebSocket 网关
│   │   ├── ws_server.h          # WebSocket 服务器 API
│   │   └── ws_server.c          # ESP HTTP 服务器，支持 WS 升级
│   │
│   ├── proxy/                   # HTTP 代理
│   │   ├── http_proxy.h         # 代理连接 API
│   │   └── http_proxy.c         # HTTP CONNECT 隧道 + TLS
│   │
│   ├── cli/                     # 串行 CLI
│   │   ├── serial_cli.h         # CLI 初始化 API
│   │   └── serial_cli.c         # esp_console REPL 调试命令
│   │
│   ├── ota/                     # OTA 更新
│   │   ├── ota_manager.h        # OTA 更新 API
│   │   └── ota_manager.c        # esp_https_ota 包装
│   │
│   ├── Audio/                   # 音频子系统
│   │   ├── audio_stream.c       # 音频流逻辑
│   │   ├── i2s_mic.c            # I2S 麦克风驱动
│   │   └── ws_client.c          # WebSocket 音频客户端
│   │
│   └── IMU/                     # IMU 传感器
│       ├── I2C_Driver.c         # I2C 驱动
│       ├── QMI8658.c            # IMU 传感器驱动
│       └── imu_manager.c        # IMU 管理器
│
├── docs/                        # 项目文档目录
│   ├── ARCHITECTURE.md          # 系统架构设计文档
│   ├── DEVELOPMENT_GUIDE.md     # 开发指南（本文件）
│   ├── FEATURES.md              # 功能清单
│   └── INSTALL_MACOS.md         # macOS 安装指南
│
├── ESP32-S3-Touch-LCD-1.85C-Test/   # LCD 测试参考代码
│   └── main/
│       ├── LCD_Driver/          # ST77916 LCD 原始代码
│       ├── LVGL_Driver/         # LVGL UI 原始代码
│       └── main.c               # 测试入口
│
├── spiffs_data/                 # SPIFFS 文件系统数据
│   └── config/                  # 配置文件目录
│   └── memory/                  # 记忆文件目录
│   └── sessions/                # 会话历史目录
│
├── README.md                    # 项目首页
└── CMakeLists.txt               # ESP-IDF 项目根配置
```

---

## 启动流程

```
app_main()
  ├── init_nvs()                          # NVS 闪存初始化
  ├── esp_event_loop_create_default()     # ESP-IDF 事件循环
  ├── init_spiffs()                       # 挂载 SPIFFS 文件系统
  ├── message_bus_init()                  # 创建双向队列
  ├── memory_store_init()                 # SPIFFS 路径验证
  ├── session_mgr_init()                  # 会话管理器
  ├── wifi_manager_init()                 # WiFi STA 模式 + 事件
  ├── http_proxy_init()                   # 代理配置加载
  ├── telegram_bot_init()                 # Bot 令牌加载
  ├── llm_proxy_init()                    # API 密钥 + 模型加载
  ├── tool_registry_init()                # 工具注册，JSON 构建
  ├── agent_loop_init()                   # Agent 循环初始化
  ├── serial_cli_init()                   # 串口控制台启动
  │
  ├── wifi_manager_start()                # 连接 WiFi（30s 超时）
  │
  └── [WiFi 连接后]
      ├── telegram_bot_start()            # tg_poll 任务启动（核心 0）
      ├── agent_loop_start()              # agent_loop 任务启动（核心 1）
      ├── ws_server_start()               # httpd 服务器开始（18789 端口）
      └── outbound_dispatch task          # 出站任务启动（核心 0）
```

---

## GUI 显示子系统详解

### 硬件配置

- **显示屏**: ST77916 LCD 驱动芯片
- **分辨率**: 360x360 像素（圆形显示）
- **接口**: QSPI 双通道
- **时钟频率**: 40MHz (优化色彩显示)
- **颜色顺序**: RGB
- **刷新模式**: PARTIAL (减少闪烁)

### 核心文件

#### `main/ui/ui_display.c`

显示驱动器实现，负责：

1. **初始化 LCD 面板**
   - 设置 SPI 总线
   - 配置 ST77916 驱动芯片初始化命令
   - 设置旋转参数 (swap_xy=true, mirror=false)

2. **LVGL 刷新回调**
   ```c
   void ui_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
   ```
   - 将 LVGL 帧缓冲区绘制到 LCD

3. **参数定义**
   ```c
   #define LCD_H_RES           360
   #define LCD_V_RES           360
   #define LCD_BIT_PER_PIXEL   16
   ```

#### `main/ui/ui_main.c`

主 UI 宿主，负责：

1. **LVGL 初始化**
   ```c
   lv_display_set_buffers(disp, buf1, buf2, ...)
   lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270)
   ```

2. **屏幕管理**
   - Chat 聊天屏幕
   - Status 状态屏幕
   - Skills 技能屏幕
   - Reminder 提醒屏幕
   - Music 音乐屏幕

3. **任务调度**
   - LVGL tick 定时器（2ms 周期）
   - UI 主任务循环

### 显示刷新流程

```
用户操作/事件触发
     ↓
UI 状态更新 (ui_state)
     ↓
LVGL 渲染引擎
     ↓
刷新区域计算 (ui_display_flush)
     ↓
LCD 驱动 (ui_display.c)
     ↓
ST77916 芯片输出
```

---

## 内存预算

| 组件                  | 位置       | 大小     |
|---------------------|-----------|---------|
| FreeRTOS 任务栈      | 内部 SRAM  | ~40 KB  |
| WiFi 缓冲区           | 内部 SRAM  | ~30 KB  |
| TLS 连接 (Telegram + Claude) | PSRAM      | ~120 KB |
| JSON 解析缓冲区        | PSRAM      | ~32 KB  |
| 会话历史缓存          | PSRAM      | ~32 KB  |
| 系统提示缓冲区        | PSRAM      | ~16 KB  |
| LLM 响应流缓冲区        | PSRAM      | ~32 KB  |
| 剩余可用              | PSRAM      | ~7.7 MB |

---

## 工具系统

### 现有工具

#### `web_search` (网络搜索)

使用 Brave Search API 进行联网搜索：

```c
// 工具定义
{
  "name": "web_search",
  "description": "Search the web for current information.",
  "input_schema": {
    "type": "object",
    "properties": {
      "query": {"type": "string"}
    },
    "required": ["query"]
  }
}
```

### 添加新工具

1. 定义工具结构（`tools/tool_registry.h`）
2. 实现工具逻辑（执行 API 调用、返回结果）
3. 注册到工具管理器

参考 `tools/tool_web_search.c` 实现模式。

---

## 调试命令

串口 CLI 提供的调试和维护命令：

| 命令                      | 描述                    |
|-------------------------|-----------------------|
| `wifi_status`           | 显示连接状态和 IP 地址     |
| `memory_read`           | 打印 MEMORY.md 内容      |
| `memory_write <内容>`   | 覆盖 MEMORY.md 文件      |
| `session_list`          | 列出所有会话文件           |
| `session_clear <ID>`    | 删除指定会话文件           |
| `heap_info`             | 显示内部 SRAM 和 PSRAM 信息 |
| `restart`               | 重启设备                |
| `help`                  | 显示所有可用命令           |

---

## 调试技巧

### 日志查看

```bash
# 在烧录时实时查看日志
idf.py -p /dev/ttyUSB0 monitor
```

### 内存问题排查

```bash
# 查看内存使用情况
mimi> heap_info

# 输出示例:
Internal SRAM free: 45232 bytes (45.2 KB)
PSRAM free: 8386432 bytes (8.38 MB)
```

### WiFi 诊断

```bash
# 检查 WiFi 连接
mimi> wifi_status

# 输出示例:
WiFi status: CONNECTED
IP address: 192.168.1.100
RSSI: -52 dBm
```

---

## OTA 更新

支持通过 HTTPS 进行固件更新：

1. 将新固件上传到 HTTPS 服务器
2. 调用 `ota_download` 函数
3. 设备下载安装并重启

---

## 参考资料

### 外部资源

- [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/)
- [LVGL UI 库](https://docs.lvgl.io/)
- [ST77916 LCD 数据手册](https://www.lcdwiki.com/ST77916_HD4308_1.8)
- [Anthropic API 文档](https://docs.anthropic.com/claude/reference/getting-started-with-the-api)
- [OpenClaw 项目](https://github.com/openclaw/openclaw)
- [Nanobot 参考实现](https://github.com/HKUDS/nanobot)

### 项目文档索引

#### 📚 核心开发文档

| 文档 | 描述 | 链接 |
|------|------|------|
| **README.md** | 项目主文档，快速开始 | [📄 docs/README.md](../README.md) |
| **PROJECT_SUMMARY.md** | 项目完整说明和元数据 | [📄 docs/PROJECT_SUMMARY.md](./PROJECT_SUMMARY.md) |
| **DEVELOPMENT_GUIDE.md** | 开发指南（本文件） | [📄 docs/DEVELOPMENT_GUIDE.md](./DEVELOPMENT_GUIDE.md) |
| **ARCHITECTURE.md** | 系统架构设计文档 | [📄 docs/ARCHITECTURE.md](./ARCHITECTURE.md) |
| **ARCHITECTURE_DIAGRAMS.md** | 6 个可视化架构图 | [📊 docs/ARCHITECTURE_DIAGRAMS.md](./ARCHITECTURE_DIAGRAMS.md) |

#### 📋 功能与设计文档

| 文档 | 描述 | 链接 |
|------|------|------|
| **FEATURES.md** | 功能清单 | [📄 docs/FEATURES.md](./FEATURES.md) |
| **FEATURES_CN.md** | 功能清单（中文） | [📄 docs/FEATURES_CN.md](./FEATURES_CN.md) |
| **TODO.md** | 功能路线图和差距追踪 | [📄 docs/TODO.md](./TODO.md) |
| **INSTALL_MACOS.md** | macOS 安装指南 | [📄 docs/INSTALL_MACOS.md](./INSTALL_MACOS.md) |

#### 📝 UI/UX 设计文档

| 文档 | 描述 | 链接 |
|------|------|------|
| **2026-03-07-ui-redesign.md** | UI 重设计计划 | [📄 docs/plans/2026-04-05-ui-redesign.md](./plans/2026-04-05-ui-redesign.md) |
| **2026-03-07-funasr-voice-streaming.md** | FunASR 语音流集成 | [📄 docs/plans/2026-03-07-funasr-voice-streaming.md](./plans/2026-03-07-funasr-voice-streaming.md) |
| **2026-02-26-display-driver-improvement.md** | 显示驱动改进 | [📄 docs/plans/2026-02-26-display-driver-improvement.md](./plans/2026-02-26-display-driver-improvement.md) |
| **2026-02-25-llm-chat-round-display.md** | LLM 聊天圆形显示器 | [📄 docs/plans/2026-02-25-llm-chat-round-display.md](./plans/2026-02-25-llm-chat-round-display.md) |
| **2026-02-24-lvgl-round-display-design.md** | LVGL 圆屏设计 | [📄 docs/plans/2026-02-24-lvgl-round-display-design.md](./plans/2026-02-24-lvgl-round-display-design.md) |

#### 🔬 测试文档

| 文档 | 描述 | 链接 |
|------|------|------|
| **UI_REDESIGN_TEST_PLAN.md** | UI 重设计测试计划 | [📄 docs/testing/UI_REDESIGN_TEST_PLAN.md](./testing/UI_REDESIGN_TEST_PLAN.md) |

---

## 快速导航

### 按开发阶段选择文档

#### 🚀 开始开发前

1. **阅读 README.md** - 了解项目背景
2. **查看 PROJECT_SUMMARY.md** - 获取完整项目概览
3. **浏览 ARCHITECTURE_DIAGRAMS.md** - 理解架构设计

#### 🛠️ 开始写代码

4. **阅读 DEVELOPMENT_GUIDE.md** - 开发环境和构建流程
5. **查看 ARCHITECTURE.md** - 详细系统架构
6. **参考 FEATURES.md** - 了解当前功能状态

#### 📐 设计新特性

7. **查看 TODO.md** - 避免重复实现
8. **参考 UI/UX 设计文档** - 了解已有设计
9. **阅读功能设计计划** - 学习设计决策

#### 🎨 调试和维护

10. **参考调试技巧** - 定位问题
11. **查看测试文档** - 验证功能

---

## 文档维护指南

本文档使用 Markdown 格式，建议：

- **更新频率**：每月或重大发布后更新
- **版本标记**：在文档开头添加 `版本：X.Y` 和 `最后更新：YYYY-MM-DD`
- **链接检查**：定期验证所有链接的有效性
- **格式规范**：使用统一的标题层级和语法高亮

### 文档结构标准

```markdown
# 标题

> 副标题 - 简短描述

---

## 章节 1
内容...

## 章节 2
内容...

---

## 最后更新：YYYY-MM-DD
```

### 链接命名规范

- 使用相对链接：`[链接文本](./filename.md)`
- GIF 图标标识：`📄 文件名` (文档), `📊 文件名` (图表), `🔧 文件名` (配置)
- 链接类型：内部分 `./file.md`，外部 `https://...`

---


## 故障排除

### LCD 显示问题

**问题**: 屏幕显示偏色或模糊

**解决方案**:
1. 检查 LCD 初始化参数：`lcd_cmd_bits=32`, `lcd_param_bits=8`
2. 确认颜色顺序：`LCD_RGB_ELEMENT_ORDER_RGB`
3. 调整时钟频率：从 80MHz 降低到 40MHz 可改善色彩

**常见配置**:
- 时钟：40MHz
- 颜色顺序：RGB
- 反转：禁用
- 旋转：swap_xy=true, mirror=false,false

### 编译错误

**问题**: `idf.py build` 失败

**解决方案**:
1. 检查 ESP-IDF 版本：必须 v5.5+
2. 清理构建：`idf.py fullclean`
3. 检查配置文件：确保 `mimi_secrets.h` 存在且正确

### 烧录失败

**问题**: `idf.py flash` 失败

**解决方案**:
1. 检查串口设备权限：`ls -l /dev/ttyUSB*`
2. 尝试不同波特率：`460800` 或 `115200`
3. 更换 USB 端口：使用 USB-C 端口而非 COM 端口

---

**最后更新**: 2026-04-05

---

**注意**: 本文为开发文档，持续更新中。如发现错误或有建议，请提出 PR。