# UI Redesign Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Completely redesign the UI layout and improve reliability by replacing the current sector-based UI with a modern, clean design.

**Architecture:** Replace the current circular sector-based interface with a simplified vertical scrolling layout using LVGL frames. Reduce complexity by consolidating screen management, improving error handling, and adding proper cleanup/initialization routines.

**Tech Stack:** LVGL (Light and Versatile Graphics Library), ESP32-S3, ST77916 LCD Display

---

## Executive Summary

### Current Architecture Analysis

**Screen Management:**
- 6 fixed circular sectors with trigonometric positioning
- `show_screen()` function controls visibility
- All screens created simultaneously, hidden by default
- No proper cleanup or error handling

**Critical Reliability Issues:**

1. **Memory Leaks**: No LVGL object cleanup or deinit
2. **No Error Handling**: No NULL checks for malloc/LVGL returns
3. **Task Watchdog Triggers**: Boot logs show IDLE0 watchdog firing
4. **Race Conditions**: Multiple screen visibility toggles in sequence
5. **No Robust Init**: No timeout or validation

**Current Init Sequence Problems:**
```
1. ui_state_init() - Global state
2. lv_init() - LVGL library
3. ui_display_init() - Display driver
4. buffer allocations (static, no check) - FAILS HERE ON WEAK MEMORY
5. create_main_screen() - Complex layout creation
6. Time starts - 500ms+ to complete
```

### New UI Goals

1. **Screen Manager Pattern**: Single centralized manager
2. **Modular Design**: Independent screen components
3. **Robust Error Handling**: Check all allocations and LVGL returns
4. **Proper Cleanup**: Deinit all resources
5. **Simplified Layout**: Vertical scrolling, no complex geometry

---

## Phase 1: Current UI Deep Analysis (COMPLETED)

### Task 1a: Document Current Architecture
- Sector-based navigation with 6 fixed buttons
- `show_screen()` switches between 5 screens
- Global `current_screen` variable tracks active screen
- All LVGL objects created at init, no cleanup

### Task 1b: Identify Reliability Issues
- No error handling for `heap_caps_malloc()`
- No cleanup/deinit functions
- Task watchdog triggers on startup
- Potential race conditions in screen toggling
- Static allocations may not initialize correctly

### Task 1c: Define New UI Goals
- Centralized screen manager
- Uniform screen interface
- Comprehensive error checking
- Proper lifecycle management

---

## Phase 2: New UI Design

### Task 2: Design New Vertical Scroll Layout

**Files:**
- Create: `main/ui/screen_manager.h`
- Create: `main/ui/screen_manager.c`
- Create: `main/ui/ui_main_new.c`

**Step 1: Define new screen structure**

Centralized Screen Manager:
```c
typedef struct {
    lv_disp_t *disp;
    lv_obj_t *container;
    lv_obj_t *header;
    lv_obj_t *footer;
    bool active_screen;
    uint8_t screen_count;
} screen_manager_t;

// Initialization
screen_manager_t* screen_manager_init(lv_disp_t *disp);
void screen_manager_destroy(screen_manager_t *mgr);

// Screen switching
void screen_manager_show(screen_manager_t *mgr, uint8_t screen_id);
```

**Step 2: Define uniform screen interface**

All screens follow same pattern:
```c
// Init
esp_err_t screen_<name>_init(lv_disp_t *disp);
void screen_<name>_destroy();
void screen_<name>_show(void);
void screen_<name>_hide(void);
```

**Step 3: Screen layout design**

New layout approach:
- Full-screen scrollable container
- Header bar (title, status indicator)
- Footer bar (system icons)
- Message/content area with scroll
- Consistent padding and spacing

---

## Phase 3: Implementation - Core Infrastructure

### Task 3: Implement Screen Manager

**Files:**
- Create: `main/ui/screen_manager.h`
- Create: `main/ui/screen_manager.c`

**Step 1: Create screen manager header**

Header structure with all needed types and functions

**Step 2: Implement initialization**

Create screen manager that:
- Validates display handle
- Creates main container with scroll
- Sets up header/footer bars
- Initializes screen tracking

**Step 3: Implement screen switching**

Show function that:
- Validates screen ID
- Hides all current screens
- Shows target screen
- Updates active flag

**Step 4: Add error handling**

Check all LVGL return values:
- NULL pointer validation
- Object creation failure checks
- Memory allocation validation

---

## Phase 4: Implementation - Screen Components

### Task 4: Refactor Chat Screen

**Files:**
- Modify: `main/ui/ui_chat.c`
- Modify: `main/ui/ui_chat.h`

**Step 1: Add proper init sequence**

Check all allocations, validate display handle

**Step 2: Add destroy function**

```c
void ui_chat_destroy(void) {
    if (s_scroll_cont) lv_obj_del(s_scroll_cont);
    if (s_msg_container) lv_obj_del(s_msg_container);
    s_scroll_cont = NULL;
    s_msg_container = NULL;
    s_initialized = false;
}
```

**Step 3: Standardize show/hide**

Consistent interface with `ui_chat_show(void)` and `ui_chat_hide(void)`

### Task 5: Refactor Status Screen

**Files:**
- Modify: `main/ui/ui_status.c`
- Modify: `main/ui/ui_status.h`

**Step 1: Add destroy function**

Same pattern as chat screen

**Step 2: Add real-time updates**

Timer-based refresh of system info (WiFi, memory, uptime)

### Task 6: Refactor Remaining Screens

Similar refactoring for:
- Skills screen
- Music screen  
- Reminder screen

---

## Phase 5: Integration and Testing

### Task 7: Replace UI Initialization

**Files:**
- Modify: `main/ui/ui_main.c`
- Modify: `main/ui/ui_main.h`

**Step 1: New init sequence**

```c
esp_err_t ui_init(void) {
    // 1. Initialize display driver first
    ESP_ERROR_CHECK(ui_display_init());
    
    // 2. Create screen manager
    s_mgr = screen_manager_init(disp);
    if (!s_mgr) return ESP_FAIL;
    
    // 3. Initialize all screens
    if (ESP_FAIL == ui_chat_init()) return ESP_FAIL;
    if (ESP_FAIL == ui_status_init()) return ESP_FAIL;
    // ... other screens
    
    // 4. Start LVGL timer
    // 5. Initialize to chat screen
    screen_manager_show(s_mgr, SCREEN_CHAT);
    
    ESP_LOGI(TAG, "UI initialized successfully");
    return ESP_OK;
}
```

**Step 2: Add deinit function**

```c
esp_err_t ui_deinit(void) {
    // Destroy all screens
    ui_chat_destroy();
    ui_status_destroy();
    // ... others
    
    // Destroy manager
    if (s_mgr) {
        screen_manager_destroy(s_mgr);
        s_mgr = NULL;
    }
    
    // Free LVGL
    lv_deinit();
    
    return ESP_OK;
}
```

### Task 8: Add Comprehensive Error Handling

**Files:**
- Modify: `main/ui/screen_manager.c`
- Add: `main/ui/ui_debug.h` (optional)

**Step 1: Add logging**

Use `ESP_LOGE` for errors, `ESP_LOGW` for warnings

**Step 2: Add null checks**

Every LVGL function return check

**Step 3: Add timeout watchdog**

Init must complete within 5 seconds

### Task 9: Testing Strategy

**Unit Tests:**
- Screen init/validation
- Screen switching logic
- Memory cleanup verification

**Integration Tests:**
- Boot to full UI
- All screen transitions
- Memory stability over time

**Performance Testing:**
- Screen transition time <500ms
- Init time <200ms
- No watchdog triggers
- Memory stable (no leaks)

---

## Execution Sequence

**Recommended order:**
1. Phase 2: Design (complete)
2. Phase 3: Screen Manager (3-5 hours)
3. Phase 4: Screen Components (8-12 hours)
4. Phase 5: Integration (4-6 hours)
5. Testing and Documentation (2-4 hours)

**Total estimated time: 3-5 man days**

---

## Success Criteria

**Completion checklist:**
- [ ] Screen manager initialized and functional
- [ ] All 5 screens follow uniform interface
- [ ] Proper deinit for all components
- [ ] Error handling everywhere
- [ ] Init time <200ms
- [ ] Screen transitions <500ms
- [ ] No task watchdog triggers
- [ ] No memory leaks
- [ ] Tested on real device
- [ ] Device boots reliably

---

## Technical Details

### Error Handling Pattern

```c
// Pattern for all LVGL calls
lv_obj_t *obj = lv_obj_create(parent);
if (obj == NULL) {
    ESP_LOGE(TAG, "Failed to create LVGL object");
    return ESP_FAIL;
}
```

### Init Sequence

```
1. Display driver init (ui_display_init())
2. Screen manager creation (screen_manager_init())
3. Screen initialization (in order)
4. LVGL timer start
5. Initial screen show
6. Completion confirmation
```

### Memory Management

- All static/global pointers initialized to NULL
- Cleanup functions called in reverse init order
- Check for NULL before operations
- lv_obj_del() for all LVGL objects

---

## Resources

**Key LVGL APIs to use:**
- `lv_obj_create()`, `lv_label_create()`
- `lv_obj_add_flag()`, `lv_obj_clear_flag()`
- `lv_obj_set_style_*()`, `lv_obj_set_flex_*()`
- `lv_obj_scroll_to_y()` for message scrolling
- `lv_obj_clean()` to remove children

**Current files to refactor:**
- `main/ui/ui_main.c` (374 lines) → Replace with cleaner version
- `main/ui/ui_chat.c` (132 lines) → Add destroy, error handling
- `main/ui/ui_status.c` (70 lines) → Add destroy, real-time updates
- `main/ui/ui_skills.c` (43 lines) → Add destroy, list handling
- `main/ui/ui_music.c` (43 lines) → Add destroy, controls
- `main/ui/ui_reminder.c` (43 lines) → Add destroy, list handling

---

## Risk Assessment

**High Risk:**
- Task still causes WDT - add timeout protection
- LVGL object creation fails - verify screen dimensions

**Medium Risk:**
- Init timing issues - separate init phases
- Memory allocation failures - graceful fallback

**Low Risk:**
- Screen layout - straightforward refactoring
- Transitions - already supported by LVGL

---

## Notes

**Don't modify:**
- `main/ui/ui_display.c` - Keep existing working implementation
- `main/ui/ui_display.h` - Keep as interface layer
- LVGL library in managed_components

**Keep consistent:**
- Color palette across all screens
- Font choices (Montserrat 14)
- Padding and spacing patterns
- Error logging style

---

**Plan complete and saved to `docs/plans/2026-04-05-ui-redesign.md`. Two execution options:**

**1. Subagent-Driven (this session)** - I dispatch fresh subagent per task, review between tasks, fast iteration

**2. Parallel Session (separate)** - Open new session with executing-plans, batch execution with checkpoints

Which approach?
