# macOS ESP32-S3 开发环境安装指南

本指南详细介绍如何在 macOS 上安装 ESP32-S3 开发环境。MimiClaw 使用 ESP-IDF v5.5.2。

## 前置要求

### 硬件

- ESP32-S3 开发板（16MB Flash + 8MB PSRAM）
- USB Type-C 数据线

### 软件

| 组件 | 版本 | 安装方式 |
|------|------|----------|
| macOS | 12+ | - |
| Xcode Command Line Tools | 最新 | `xcode-select --install` |
| Homebrew | 最新 | [brew.sh](https://brew.sh) |
| Python | >= 3.10 | Homebrew |

---

## 安装步骤

### 1. 安装 Xcode Command Line Tools

```bash
xcode-select --install
```

弹出提示时点击「安装」。

### 2. 安装 Homebrew

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 3. 安装系统依赖

```bash
brew install git cmake ninja ccache python3 flex bison gperf dfu-util
brew install libusb libffi openssl@3
```

### 4. 克隆 ESP-IDF

```bash
export ESP_ROOT="$HOME/.espressif"
mkdir -p "$ESP_ROOT"

git clone --depth 1 --branch v5.5.2 --recursive \
  https://github.com/espressif/esp-idf.git "$ESP_ROOT/esp-idf-v5.5.2"
```

### 5. 安装 ESP32-S3 工具链

```bash
cd "$ESP_ROOT/esp-idf-v5.5.2"
./install.sh esp32s3
```

### 6. 设置环境变量

每次使用前执行：

```bash
. "$HOME/.espressif/esp-idf-v5.5.2/export.sh"
```

或添加别名到 `~/.zshrc`：

```bash
echo 'alias get_idf="$HOME/.espressif/esp-idf-v5.5.2/export.sh"' >> ~/.zshrc
source ~/.zshrc
```

---

## 验证安装

```bash
# 激活环境
. "$HOME/.espressif/esp-idf-v5.5.2/export.sh"

# 检查版本
idf.py --version
# 输出: v5.5.2

# 查看支持的芯片
idf.py --list-chips
# 输出包含: esp32s3
```

---

## 快速开始 MimiClaw

```bash
# 1. 克隆项目
git clone https://github.com/memovai/mimiclaw.git
cd mimiclaw

# 2. 配置密钥
cp main/mimi_secrets.h.example main/mimi_secrets.h
# 编辑 main/mimi_secrets.h 填入 WiFi、Telegram Token、API Key

# 3. 设置目标并编译
idf.py set-target esp32s3
idf.py build

# 4. 查找串口
ls /dev/cu.usb*

# 5. 烧录
idf.py -p /dev/cu.usb* flash monitor
```

---

## 常见问题

### Q: 找不到串口

1. 确认使用 USB 接口（非 COM 接口）
2. 确认是数据线（非仅供电线）
3. 安装驱动：`brew install libusb`

### Q: 烧录失败

按住 **BOOT** 按钮，点击 **EN** 按钮，然后松开 **BOOT**。

### Q: 权限问题

```bash
sudo chown -R $(whoami) /dev/cu.usb*
```

---

## 相关链接

- [ESP-IDF 官方文档](https://docs.espressif.com/projects/esp-idf/en/v5.5.2/esp32s3/get-started/)
- [MimiClaw GitHub](https://github.com/memovai/mimiclaw)
