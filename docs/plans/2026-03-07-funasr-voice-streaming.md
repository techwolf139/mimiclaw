# FunASR Voice Forwarding Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 实现ESP32-S3通过WebSocket持续将麦克风音频流转发给外部FunASR服务进行语音识别

**Architecture:** 添加I2S麦克风驱动采集音频,通过WebSocket客户端将PCM数据流式转发给FunASR服务器。参考ESP32-S3-Touch-LCD-1.85C-Test项目的MIC_Speech.c实现

**Tech Stack:** ESP-IDF v5.5, I2S驱动, esp_websocket_client, FunASR WebSocket协议

---

## 前置条件

1. FunASR服务器在外部主机运行:
   ```bash
   cd /path/to/FunASR/runtime/python/websocket
   python funasr_wss_server.py --port 10095
   ```

2. ESP32-S3与FunASR服务器在同一网络

---

## Task 1: 添加 I2S 麦克风驱动

**Files:**
- Create: `main/audio/i2s_mic.c`
- Create: `main/audio/i2s_mic.h`
- Modify: `main/CMakeLists.txt` - 添加 i2s_mic.c 编译

**Step 1: 创建 i2s_mic.h**

```c
#ifndef I2S_MIC_H
#define I2S_MIC_H

#include "esp_err.h"

#define I2S_MIC_SAMPLE_RATE    16000
#define I2S_MIC_CHANNEL_NUM    1

typedef void (*i2s_mic_data_callback_t)(const uint8_t *data, size_t len);

esp_err_t i2s_mic_init(void);
esp_err_t i2s_mic_start(void);
esp_err_t i2s_mic_stop(void);
esp_err_t i2s_mic_deinit(void);
esp_err_t i2s_mic_register_callback(i2s_mic_data_callback_t callback);

#endif
```

**Step 2: 创建 i2s_mic.c**

参考 `/Users/mac/Downloads/ESP32-S3-Touch-LCD-1.85C-Demo/ESP-IDF/ESP32-S3-Touch-LCD-1.85C-Test/main/MIC_Driver/MIC_Speech.c` 中的I2S初始化代码:
- 使用 I2S 标准模式
- 采样率 16000 Hz
- 单声道 (MONO)
- 32位数据宽度 (实际16位有效)

关键代码:
```c
#include "driver/i2s_std.h"
#include "driver/gpio.h"

static i2s_chan_handle_t rx_handle = NULL;

esp_err_t i2s_mic_init(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    
    i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, I2S_DATA_BIT_WIDTH_32BIT);
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    
    return ESP_OK;
}
```

**Step 3: 更新 CMakeLists.txt**

在 `main/CMakeLists.txt` 添加:
```cmake
"audio/i2s_mic.c",
```

**Step 4: 编译验证**

```bash
idf.py build
```

Expected: Build succeeds

---

## Task 2: 添加 WebSocket 客户端

**Files:**
- Create: `main/audio/ws_client.c`
- Create: `main/audio/ws_client.h`
- Modify: `main/CMakeLists.txt`

**Step 1: 创建 ws_client.h**

```c
#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include "esp_err.h"

typedef void (*ws_client_text_callback_t)(const char *text);
typedef void (*ws_client_connect_callback_t)(bool connected);

esp_err_t ws_client_init(const char *host, int port);
esp_err_t ws_client_connect(void);
esp_err_t ws_client_disconnect(void);
esp_err_t ws_client_send_audio(const uint8_t *data, size_t len);
esp_err_t ws_client_register_text_callback(ws_client_text_callback_t callback);
esp_err_t ws_client_register_connect_callback(ws_client_connect_callback_t callback);

#endif
```

**Step 2: 创建 ws_client.c**

使用 esp_websocket_client:
```c
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "string.h"

static esp_websocket_client_handle_t client = NULL;
static ws_client_text_callback_t text_cb = NULL;
static ws_client_connect_callback_t connect_cb = NULL;

static void ws_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        if (connect_cb) connect_cb(true);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        if (connect_cb) connect_cb(false);
        break;
    case WEBSOCKET_EVENT_DATA:
        if (data->data_len > 0 && text_cb) {
            // 处理识别结果
            text_cb((const char *)data->data_ptr);
        }
        break;
    }
}

esp_err_t ws_client_init(const char *host, int port) {
    esp_websocket_client_config_t config = {
        .uri = "ws://localhost:10095",
    };
    client = esp_websocket_client_init(&config);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
    return ESP_OK;
}
```

**Step 3: 更新 CMakeLists.txt**

添加:
```cmake
"audio/ws_client.c",
```

**Step 4: 编译验证**

```bash
idf.py build
```

Expected: Build succeeds

---

## Task 3: 集成音频流转发

**Files:**
- Create: `main/audio/audio_stream.c`
- Modify: `main/CMakeLists.txt`
- Modify: `main/mimi.c` - 启动语音识别

**Step 1: 创建 audio_stream.c**

```c
#include "i2s_mic.h"
#include "ws_client.h"
#include "esp_log.h"

static const char *TAG = "audio_stream";

static void mic_data_callback(const uint8_t *data, size_t len) {
    ws_client_send_audio(data, len);
}

static void text_result_callback(const char *text) {
    ESP_LOGI(TAG, "ASR result: %s", text);
    // 发送识别结果到消息总线
}

static void connect_callback(bool connected) {
    ESP_LOGI(TAG, "WS %s", connected ? "connected" : "disconnected");
}

esp_err_t audio_stream_init(const char *funasr_host, int funasr_port) {
    // 初始化麦克风
    ESP_ERROR_CHECK(i2s_mic_init());
    ESP_ERROR_CHECK(i2s_mic_register_callback(mic_data_callback));
    
    // 初始化WebSocket客户端
    ESP_ERROR_CHECK(ws_client_init(funasr_host, funasr_port));
    ESP_ERROR_CHECK(ws_client_register_text_callback(text_result_callback));
    ESP_ERROR_CHECK(ws_client_register_connect_callback(connect_callback));
    
    return ESP_OK;
}

esp_err_t audio_stream_start(void) {
    ESP_ERROR_CHECK(i2s_mic_start());
    return ws_client_connect();
}

esp_err_t audio_stream_stop(void) {
    i2s_mic_stop();
    ws_client_disconnect();
    return ESP_OK;
}
```

**Step 2: 更新 CMakeLists.txt**

添加:
```cmake
"audio/audio_stream.c",
```

**Step 3: 在 mimi.c 中集成**

在 app_main() 中添加:
```c
#include "audio/audio_stream.h"

// WiFi连接成功后启动
if (wifi_manager_wait_connected(30000) == ESP_OK) {
    // 启动语音识别
    audio_stream_init("192.168.x.x", 10095);  // FunASR服务器IP
    audio_stream_start();
}
```

**Step 4: 编译验证**

```bash
idf.py build
```

Expected: Build succeeds

---

## Task 4: 添加配置项

**Files:**
- Modify: `main/mimi_config.h` - 添加FunASR服务器配置

**Step 1: 添加配置项**

```c
// FunASR服务器配置
#ifndef MIMI_SECRET_FUNASR_HOST
#define MIMI_SECRET_FUNASR_HOST "192.168.1.100"
#endif

#ifndef MIMI_SECRET_FUNASR_PORT
#define MIMI_SECRET_FUNASR_PORT 10095
#endif
```

**Step 2: 在 mimi_secrets.h.example 中添加说明**

```c
// FunASR语音识别服务器
#define MIMI_SECRET_FUNASR_HOST "192.168.1.100"  // FunASR服务器IP地址
#define MIMI_SECRET_FUNASR_PORT 10095              // FunASR WebSocket端口
```

---

## Task 5: 测试验证

**Step 1: 启动FunASR服务器**

在有GPU的机器上:
```bash
cd /path/to/FunASR/runtime/python/websocket
python funasr_wss_server.py --port 10095
```

**Step 2: 烧录并运行**

```bash
idf.py flash monitor
```

**Step 3: 验证**

- 麦克风开始采集音频
- WebSocket连接到FunASR服务器
- 说话时终端显示识别结果

---

## FunASR WebSocket 协议说明

FunASR服务器期望:
1. 连接建立后发送音频数据 (二进制 PCM)
2. 服务器返回识别结果 (JSON格式)

参考: `/Users/mac/.config/opencode/skills/funasr-asr/scripts/funasr_ws_client.py`

---

## 提交更改

```bash
git add main/audio/ main/mimi.c main/mimi_config.h
git commit -m "feat(audio): add voice streaming to FunASR

- Add I2S microphone driver for audio capture
- Add WebSocket client for streaming to FunASR server
- Add audio stream integration module
- Add FunASR server configuration options"
```

---

## Plan complete

**Two execution options:**

1. **Subagent-Driven (this session)** - I dispatch fresh subagent per task, review between tasks, fast iteration

2. **Parallel Session (separate)** - Open new session with executing-plans, batch execution with checkpoints

Which approach?
