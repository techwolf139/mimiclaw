# 1.85" Round LCD UI Design for MimiClaw

## Overview

Add a 1.85" 360×360 round LCD display to MimiClaw (ESP32-S3) for status display and touch interaction.

## Hardware

| Component | Specification |
|-----------|---------------|
| Display | Waveshare ESP32-S3-Touch-LCD-1.85C |
| Resolution | 360×360 (round) |
| Driver IC | ST7789P |
| Touch IC | CST816D |
| Interface | SPI |

## UI Layout

```
┌─────────────────────┐
│                     │
│     ╭─────────╮     │
│     │         │     │
│     │  可滚动  │     │  ← 圆形主内容区 (可滚动)
│     │  内容区  │     │
│     │         │     │
│     ╰─────────╯     │
│                     │
└─────────────────────┘
```

- Effective area: ~254×254px (circle inscribed square)
- Scrollable vertically when content exceeds display area

## Display Content

1. **WiFi Status** - Connection state + RSSI
2. **Message Count** - Telegram messages received
3. **Uptime** - Running time (HH:MM:SS)
4. **Conversation Preview** - Last 1-2 messages
5. **Activity Timeline** - Recent activity log (5 items)

## Architecture

```
┌─────────────────────────────────────────────┐
│               UI Layer (LVGL)                │
│  ┌─────────────────────────────────────────┐│
│  │            lvgl_task                     ││
│  │  ┌───────────────────────────────────┐  ││
│  │  │      MainScreen                   │  ││
│  │  │  ┌─────────────────────────────┐  │  ││
│  │  │  │   ScrollContainer            │  │  ││
│  │  │  │   - StatusRow                │  │  ││
│  │  │  │   - ConversationPreview      │  │  ││
│  │  │  │   - ActivityTimeline         │  │  ││
│  │  │  └─────────────────────────────┘  │  ││
│  │  └───────────────────────────────────┘  ││
│  └─────────────────────────────────────────┘│
└─────────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────┐
│           Display Driver                     │
│  ┌─────────────────────────────────────────┐│
│  │    ST7789P (SPI) + CST816D (Touch)     ││
│  └─────────────────────────────────────────┘│
└─────────────────────────────────────────────┘
```

## File Structure

```
main/
├── ui/
│   ├── ui_main.c/h        # LVGL task entry
│   ├── ui_display.c/h    # ST7789P driver
│   ├── ui_touch.c/h      # CST816 driver
│   ├── ui_screen.c/h     # Screen components
│   ├── ui_state.c/h      # State management
│   └── ui_theme.c/h      # Theme/styles
└── mimi.c               # app_main() adds ui_init()
```

## Memory Budget

| Component | Budget |
|-----------|--------|
| LVGL base (disp/indev) | ~40KB |
| UI state struct | ~2KB |
| Double buffer (360×360×2) | ~250KB (PSRAM) |
| Stack (lvgl task) | ~4KB |
| **Total** | ~300KB |

## MVP Features

1. ✅ LVGL integration
2. ✅ ST7789P display driver
3. ✅ CST816 touch driver (scroll only)
4. ✅ Round display mask
5. ✅ Status display (WiFi/Count/Uptime)
6. ✅ Vertical scrolling

## Implementation Steps

1. Configure ESP-IDF with LVGL component
2. Create ui/ module structure
3. Implement ST7789P driver
4. Implement CST816 driver
5. Integrate LVGL with round screen mask
6. Build status display components
7. Integrate into mimi.c
8. Build and verify

---

Created: 2026-02-24
