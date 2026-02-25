# LLM Chat Display on Round LCD Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Display LLM chat messages on the 1.85" round LCD display with improved visual quality and interactive scrolling.

**Architecture:** Add chat UI components to existing LVGL-based UI framework. Integrate with message bus to receive real-time chat updates. Use scrollable container for chat history with circular mask for round display.

**Tech Stack:** ESP32-S3, LVGL, ST77916 LCD Driver, cJSON

---

## Overview

This plan adds LLM chat display functionality to the existing round LCD UI:

1. **Message Bus Integration** - Connect to existing message bus to receive chat updates
2. **Chat UI Components** - Create scrollable chat history view
3. **Improved Typography** - Add better fonts and text rendering for readability
4. **Interactive Features** - Touch scrolling and message input

### File Structure

```
main/
├── ui/
│   ├── ui_main.c/h         # Existing - add chat screen
│   ├── ui_chat.c/h        # NEW - chat component
│   ├── ui_display.c/h     # Existing - LCD driver
│   ├── ui_state.c/h       # Existing - state management
│   ├── ui_touch.c/h       # Existing - touch input
│   └── fonts/             # NEW - add fonts
├── bus/
│   └── message_bus.c/h    # Existing - integrate with this
├── mimi.c                 # Existing - hook up chat UI
└── CMakeLists.txt        # Modify - add new files
```

---

## Tasks

### Task 1: Add Chat Message Structure to UI State

**Files:**
- Modify: `main/ui/ui_state.h:12-34`
- Modify: `main/ui/ui_state.c:1-60`

**Step 1: Add chat message structure**

In `ui_state.h`, add after `touch_pressed` field:

```c
// Chat message (max 20 messages stored)
#define UI_CHAT_MAX_MESSAGES 20
#define UI_CHAT_MSG_MAX_LEN  256

typedef struct {
    char role[16];        // "user" or "assistant"
    char content[UI_CHAT_MSG_MAX_LEN];
    uint32_t timestamp;
} ui_chat_msg_t;

typedef struct {
    // ... existing fields ...
    
    // Chat messages
    ui_chat_msg_t chat_messages[UI_CHAT_MAX_MESSAGES];
    int chat_msg_count;
    int chat_scroll_pos;
    
    // Chat state
    bool chat_active;
    bool receiving_response;
} ui_state_t;
```

**Step 2: Add chat state functions to ui_state.c**

Add after existing functions:

```c
void ui_state_add_chat_message(const char *role, const char *content) {
    if (!role || !content) return;
    
    ui_chat_msg_t *msg = &g_ui_state.chat_messages[g_ui_state.chat_msg_count % UI_CHAT_MAX_MESSAGES];
    snprintf(msg->role, sizeof(msg->role), "%s", role);
    snprintf(msg->content, sizeof(msg->content), "%.255s", content);
    msg->timestamp = g_ui_state.uptime_seconds;
    
    if (g_ui_state.chat_msg_count < UI_CHAT_MAX_MESSAGES) {
        g_ui_state.chat_msg_count++;
    }
}

void ui_state_clear_chat(void) {
    g_ui_state.chat_msg_count = 0;
    g_ui_state.chat_scroll_pos = 0;
}
```

**Step 3: Add function declarations to ui_state.h**

Add before `#ifdef __cplusplus`:

```c
// Chat message functions
void ui_state_add_chat_message(const char *role, const char *content);
void ui_state_clear_chat(void);
```

**Step 4: Build to verify**

Run: `idf.py build`
Expected: PASS (no errors)

---

### Task 2: Create Chat UI Component

**Files:**
- Create: `main/ui/ui_chat.h`
- Create: `main/ui/ui_chat.c`

**Step 1: Create ui_chat.h**

```c
#ifndef UI_CHAT_H
#define UI_CHAT_H

#include <stdint.h>
#include <esp_err.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ui_chat_init(lv_display_t *disp);
void ui_chat_update(void);
void ui_chat_show(bool show);
void ui_chat_scroll_down(void);

#ifdef __cplusplus
}
#endif

#endif
```

**Step 2: Create ui_chat.c**

```c
#include <string.h>
#include "ui_chat.h"
#include "ui_state.h"
#include "esp_log.h"

static const char *TAG = "ui_chat";

static lv_display_t *s_disp;
static lv_obj_t *s_scroll_cont;
static lv_obj_t *s_msg_container;
static bool s_initialized = false;
static bool s_visible = false;

// Colors
#define COLOR_BG         0x000000
#define COLOR_USER_BG    0x1A1A2E
#define COLOR_ASSIST_BG  0x0F3460
#define COLOR_USER_TEXT  0x00D4FF
#define COLOR_ASSIST_TEXT 0xFFFFFF
#define COLOR_TIMESTAMP  0x666666

esp_err_t ui_chat_init(lv_display_t *disp) {
    if (s_initialized) {
        return ESP_OK;
    }
    
    s_disp = disp;
    
    ESP_LOGI(TAG, "Initializing chat UI");
    
    // Get active screen
    lv_obj_t *scr = lv_screen_active();
    
    // Create scrollable container (full screen)
    s_scroll_cont = lv_obj_create(scr);
    lv_obj_set_size(s_scroll_cont, 360, 360);
    lv_obj_set_pos(s_scroll_cont, 0, 0);
    lv_obj_set_style_bg_color(s_scroll_cont, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_scroll_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(s_scroll_cont, 0, 0);
    
    // Enable scrolling
    lv_obj_set_scroll_dir(s_scroll_cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_scroll_cont, LV_SCROLLBAR_MODE_OFF);
    
    // Message container (vertical flex)
    s_msg_container = lv_obj_create(s_scroll_cont);
    lv_obj_set_size(s_msg_container, 360, 360);
    lv_obj_set_pos(s_msg_container, 0, 0);
    lv_obj_set_style_bg_opa(s_msg_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_msg_container, 0, 0);
    lv_obj_set_style_pad_all(s_msg_container, 8, 0);
    lv_obj_set_flex_flow(s_msg_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_msg_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Hide initially
    lv_obj_add_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Chat UI initialized");
    
    return ESP_OK;
}

void ui_chat_update(void) {
    if (!s_initialized || !s_visible) return;
    
    // Clear existing messages
    lv_obj_clean(s_msg_container);
    
    // Add messages from state
    for (int i = 0; i < g_ui_state.chat_msg_count; i++) {
        ui_chat_msg_t *msg = &g_ui_state.chat_messages[i];
        
        // Message bubble
        lv_obj_t *bubble = lv_obj_create(s_msg_container);
        lv_obj_set_width(bubble, 340);
        lv_obj_set_style_bg_color(bubble, lv_color_hex(
            strcmp(msg->role, "user") == 0 ? COLOR_USER_BG : COLOR_ASSIST_BG), 0);
        lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(bubble, 12, 0);
        lv_obj_set_style_border_width(bubble, 0, 0);
        lv_obj_set_style_pad_all(bubble, 12, 0);
        
        // Role label
        lv_obj_t *role_label = lv_label_create(bubble);
        lv_label_set_text(role_label, msg->role);
        lv_obj_set_style_text_color(role_label, lv_color_hex(
            strcmp(msg->role, "user") == 0 ? COLOR_USER_TEXT : COLOR_ASSIST_TEXT), 0);
        lv_obj_set_style_text_font(role_label, &lv_font_montserrat_12, 0);
        
        // Content label
        lv_obj_t *content_label = lv_label_create(bubble);
        lv_label_set_text(content_label, msg->content);
        lv_obj_set_style_text_color(content_label, lv_color_hex(COLOR_ASSIST_TEXT), 0);
        lv_obj_set_style_text_font(content_label, &lv_font_montserrat_14, 0);
        lv_obj_set_width(content_label, 316);
        lv_label_set_long_mode(content_label, LV_LABEL_LONG_WRAP);
    }
    
    // Scroll to bottom
    lv_obj_scroll_to_y(s_scroll_cont, LV_COORD_MAX, true);
}

void ui_chat_show(bool show) {
    if (!s_initialized) return;
    
    s_visible = show;
    if (show) {
        lv_obj_remove_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
        ui_chat_update();
    } else {
        lv_obj_add_flag(s_scroll_cont, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_chat_scroll_down(void) {
    if (!s_initialized) return;
    lv_obj_scroll_to_y(s_scroll_cont, LV_COORD_MAX, true);
}
```

**Step 3: Build to verify**

Run: `idf.py build`
Expected: PASS

---

### Task 3: Integrate Chat UI with Main UI

**Files:**
- Modify: `main/ui/ui_main.c:1-271`
- Modify: `main/ui/ui_main.h:1-25`

**Step 1: Add chat screen toggle**

In `ui_main.c`, add after line 30:

```c
static bool chat_mode = false;
static lv_obj_t *main_screen_cont = NULL;
```

**Step 2: Modify create_main_screen to save reference**

In `create_main_screen()`, change line 73:
```c
// Change:
lv_obj_t *cont = lv_obj_create(scr);
// To:
main_screen_cont = lv_obj_create(scr);
lv_obj_t *cont = main_screen_cont;
```

**Step 3: Add chat screen navigation**

Add before `esp_err_t ui_init(void)`:

```c
void ui_toggle_chat_mode(void) {
    chat_mode = !chat_mode;
    
    if (chat_mode) {
        lv_obj_add_flag(main_screen_cont, LV_OBJ_FLAG_HIDDEN);
        ui_chat_show(true);
    } else {
        lv_obj_remove_flag(main_screen_cont, LV_OBJ_FLAG_HIDDEN);
        ui_chat_show(false);
    }
}
```

**Step 4: Initialize chat in ui_init**

In `ui_init()`, add after line 229 (after ui_touch_register_lvgl_indev):

```c
ui_chat_init(disp);
```

**Step 5: Add toggle handler for arc click**

In `create_main_screen()`, find the status_arc (around line 97-106) and add click handler:

```c
static void status_arc_clicked(lv_event_t *e) {
    ui_toggle_chat_mode();
}

// Add after arc setup:
lv_obj_add_event_cb(status_arc, status_arc_clicked, LV_EVENT_CLICKED, NULL);
lv_obj_clear_flag(status_arc, LV_OBJ_FLAG_CLICKABLE);  // Remove this line
```

Actually, let's add a dedicated button instead. Add before test_btn:

```c
// Chat toggle button (top right)
lv_obj_t *chat_btn = lv_btn_create(cont);
lv_obj_set_size(chat_btn, 40, 40);
lv_obj_set_style_bg_color(chat_btn, lv_color_hex(0x333333), 0);
lv_obj_set_style_radius(chat_btn, 20, 0);

lv_obj_t *chat_icon = lv_label_create(chat_btn);
lv_label_set_text(chat_icon, "C");
lv_obj_set_style_text_color(chat_icon, lv_color_hex(0xFFFFFF), 0);
lv_obj_center(chat_icon);

lv_obj_add_event_cb(chat_btn, test_btn_clicked, LV_EVENT_CLICKED, NULL);
```

Wait, let's fix the event callback - it should call our new function:

```c
static void chat_btn_clicked(lv_event_t *e) {
    ui_toggle_chat_mode();
}

// Change test_btn callback:
lv_obj_add_event_cb(chat_btn, chat_btn_clicked, LV_EVENT_CLICKED, NULL);
```

**Step 6: Add include and declaration**

Add after line 6:
```c
#include "ui_chat.h"
```

**Step 7: Build to verify**

Run: `idf.py build`
Expected: PASS

---

### Task 4: Connect Message Bus to Chat UI

**Files:**
- Modify: `main/bus/message_bus.c` (or find where messages are processed)
- Modify: `main/telegram/telegram_bot.c`

**Step 1: Find where incoming messages are handled**

Search for where LLM responses are received:

```bash
grep -r "llm_response" main/ --include="*.c" | head -20
```

**Step 2: Add callback to message bus**

Actually, let's add a simpler integration point - in telegram_bot.c where messages are sent/received.

Add to telegram_bot.c after processing a message:

```c
// After getting LLM response, add to chat UI
if (response_text && response_text[0]) {
    ui_state_add_chat_message("assistant", response_text);
    if (chat_mode) {
        ui_chat_update();
    }
}
```

And when user sends message:

```c
// When user sends message
ui_state_add_chat_message("user", user_message);
if (chat_mode) {
    ui_chat_update();
}
```

**Step 3: Build to verify**

Run: `idf.py build`
Expected: PASS

---

### Task 5: Add Better Fonts for Readability

**Files:**
- Create: `main/ui/fonts/` directory
- Modify: `CMakeLists.txt`

**Step 1: Create fonts directory**

```bash
mkdir -p main/ui/fonts
```

**Step 2: Download or generate font**

For ESP32-LVGL, you can use:
- lv_font_conv tool to convert TTF to C array
- Or use built-in fonts with larger sizes

Let's use built-in fonts with larger sizes first - modify chat UI to use:

```c
// In ui_chat.c, change fonts:
// Role label: &lv_font_montserrat_14
// Content: &lv_font_montserrat_16 (if available) or increase line height
```

**Step 3: Adjust line spacing**

In ui_chat.c, add to bubble style:
```c
lv_obj_set_style_pad_column(bubble, 8, 0);
```

**Step 4: Build to verify**

Run: `idf.py build`
Expected: PASS

---

### Task 6: Test and Refine

**Files:**
- Test on hardware

**Step 1: Flash to device**

Run: `idf.py -p /dev/cu.usbserial-* flash monitor`

**Step 2: Verify**

1. UI starts and shows main screen
2. Click chat button (C) to switch to chat view
3. Send message via Telegram
4. Watch chat update on screen

**Step 3: Refine based on testing**

- Adjust colors
- Fix scroll issues
- Improve text wrapping
- Add timestamp display

---

## Summary

This implementation adds:

1. **Chat message storage** in UI state (20 messages max)
2. **Chat UI component** with scrollable message list
3. **Screen toggle** between main status and chat view
4. **Message bus integration** to display real-time chat
5. **Improved typography** with proper bubble layout

### Estimated Time

- Task 1: 15 minutes
- Task 2: 30 minutes
- Task 3: 20 minutes
- Task 4: 20 minutes
- Task 5: 10 minutes
- Task 6: 30 minutes

**Total: ~2 hours**

---

## Dependencies

- LVGL component (already installed)
- SPI RAM for double buffering (already configured)
- Message bus (already exists)

## Testing

Test by:
1. Building: `idf.py build`
2. Flashing: `idf.py -p PORT flash`
3. Monitoring: `idf.py -p PORT monitor`
4. Sending Telegram messages
5. Observing LCD display
