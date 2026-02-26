# ESP32-S3 Display Driver Improvement Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Improve the ST77916 LCD display driver and UI rendering for 1.85" 360x360 round LCD, fixing pixel rendering issues and font color problems.

**Architecture:** Align with reference project ESP32-S3-Touch-LCD-1.85C-Test display configuration - adjust SPI clock, buffer sizes, color format settings, and add round display masking.

**Tech Stack:** ESP-IDF v5.5, LVGL v9.5, ST77916 LCD controller, QSPI interface

---

## Issues Identified

1. **SPI Clock Too Low**: Current 3MHz vs reference 80MHz
2. **Buffer Size Mismatch**: Current 90-row vs reference (360*360/20 = 6480 pixels ≈ 18 rows)
3. **Possible Color Format Issue**: RGB byte order may need verification
4. **No Round Display Mask**: Corners outside circle may show unwanted content
5. **Font Colors**: May need adjustment for better visibility

---

## Reference Values (from ESP32-S3-Touch-LCD-1.85C-Test)

```
SPI Clock: 80 MHz
Buffer: 360*360/20 = 6480 pixels (≈18 rows at 360px wide)
Color Bits: 16 (RGB565)
Display Size: 360x360
Rotation: swap_xy=true, mirror X=true, mirror Y=false
```

---

## Current Values (mimiclaw)

```
SPI Clock: 3 MHz
Buffer: 90 rows (360*90 = 32400 pixels)
Color Bits: 16 (RGB565)
Display Size: 360x360
Rotation: swap_xy=true, mirror X=false, mirror Y=true
```

---

### Task 1: Increase SPI Clock Speed

**Files:**
- Modify: `main/ui/ui_display.c:35`

**Step 1: Change SPI clock**

```c
// Current (line 35):
#define LCD_INIT_CLK_HZ     (3 * 1000 * 1000)

// Change to (80MHz):
#define LCD_INIT_CLK_HZ     (80 * 1000 * 1000)
```

**Step 2: Verify build**

```bash
idf.py build
```

Expected: Build succeeds

---

### Task 2: Optimize LVGL Buffer Size

**Files:**
- Modify: `main/ui/ui_display.h`

**Step 1: Change buffer height**

```c
// Current (line ~15):
#define LCD_DRAW_BUFF_HEIGHT  90

// Change to (1/20 of screen = 18 rows):
#define LCD_DRAW_BUFF_HEIGHT  18
```

**Step 2: Verify build**

```bash
idf.py build
```

Expected: Build succeeds

---

### Task 3: Fix Rotation Settings

**Files:**
- Modify: `main/ui/ui_display.c:287-288`

**Step 1: Adjust rotation to match reference**

```c
// Current (lines 287-288):
esp_lcd_panel_swap_xy(panel_handle, true);
esp_lcd_panel_mirror(panel_handle, false, true);

// Change to (match reference):
esp_lcd_panel_swap_xy(panel_handle, true);
esp_lcd_panel_mirror(panel_handle, true, false);
```

**Step 2: Verify build**

```bash
idf.py build
```

Expected: Build succeeds

---

### Task 4: Add Round Display Mask

**Files:**
- Modify: `main/ui/ui_main.c`

**Step 1: Add circle mask to main screen**

After line `lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);` in `create_main_screen()`:

```c
// Add round display mask
lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

// Create a circular mask for round display
lv_obj_set_style_bg_color(main_screen_cont, lv_color_hex(0x000000), 0);
lv_obj_set_style_radius(main_screen_cont, 180, 0);
lv_obj_add_flag(main_screen_cont, LV_OBJ_FLAG_CLIPABLE);
```

Or add to ui_display.c after initialization - create a full-screen circle mask layer.

**Step 2: Verify build**

```bash
idf.py build
```

Expected: Build succeeds

---

### Task 5: Verify Font Colors and Adjust

**Files:**
- Modify: `main/ui/ui_main.c`, `main/ui/ui_chat.c`

**Step 1: Review current colors**

Check if colors appear correctly. Common issues:
- Colors swapped (red looks like blue) → Check RGB order
- Colors too dark/bright → Adjust gamma in ST77916 init

If colors are wrong, modify `vendor_specific_init` in ui_display.c (lines 81-82 gamma settings).

**Step 2: Adjust if needed**

```c
// Gamma curve adjustments (if colors appear wrong)
// Line 81-82 in ui_display.c:
{0xE0, (uint8_t []){0xF0, 0x0A, 0x10, 0x09, 0x09, 0x36, 0x35, 0x33, 0x4A, 0x29, 0x15, 0x15, 0x2E, 0x34}, 14, 0},
{0xE1, (uint8_t []){0xF0, 0x0A, 0x0F, 0x08, 0x08, 0x05, 0x34, 0x33, 0x4A, 0x39, 0x15, 0x15, 0x2D, 0x33}, 14, 0},
```

**Step 3: Build and test**

```bash
idf.py build
```

Expected: Build succeeds

---

### Task 6: Test Display Output

**Step 1: Flash to device**

```bash
idf.py flash monitor
```

**Step 2: Check display output**

Verify:
- [ ] Screen displays without pixel artifacts
- [ ] Colors appear correct (red= red, green= green, blue= blue)
- [ ] Fonts render clearly without blurring
- [ ] Round corners are masked (no content in corners)
- [ ] Rotation is correct (not inverted)

---

## Commit Changes

```bash
git add main/ui/ui_display.c main/ui/ui_display.h
git commit -m "fix(display): improve ST77916 driver settings

- Increase SPI clock from 3MHz to 80MHz
- Optimize buffer size to 18 rows (1/20 screen)
- Fix rotation settings to match reference
- Add round display masking

参考 ESP32-S3-Touch-LCD-1.85C-Test 项目优化显示驱动"
```

---

## Plan complete

**Two execution options:**

1. **Subagent-Driven (this session)** - I dispatch fresh subagent per task, review between tasks, fast iteration

2. **Parallel Session (separate)** - Open new session with executing-plans, batch execution with checkpoints

Which approach?
