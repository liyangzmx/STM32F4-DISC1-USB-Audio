# 显示模块可选化 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** 通过 CMake option `ENABLE_DISPLAY`（默认 OFF）将 OLED + LVGL 显示模块变为可选，关闭时不编译显示代码、不初始化显示外设、不创建显示任务。

**Architecture:** 编译期方案。CMakeLists.txt 中 `option(ENABLE_DISPLAY OFF)` 通过 `target_compile_definitions` 传递 `ENABLE_DISPLAY=1` 宏。所有 C 代码中显示相关部分用 `#if ENABLE_DISPLAY` 守卫。`oled_display.c` 和 LVGL 源码在 CMake 层面条件编译，未启用时不进入构建。

**Tech Stack:** STM32 HAL, FreeRTOS, LVGL v9, CMake

**细节决策:**
- `hi2c2` 变量和 `MX_I2C2_Init()` 函数体保持编译（CubeMX 自动生成代码，修改后再生会丢失），仅守卫 `main()` 中的调用，链接器会自动移除未调用函数
- OLED GPIO 管脚初始化保持编译（`GPIO_PIN_x` 常量定义和 `HAL_GPIO_WritePin` 调用无害）
- `main.h` 中 OLED 管脚宏保持定义（CubeMX 自动生成，仅常量定义无害）
- 所有 `#if` 守卫尽可能放在 `USER CODE BEGIN/END` 区域内

---

### Task 1: 修改 CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 在 `add_executable` 之后添加 `ENABLE_DISPLAY` option，并将显示相关源码和路径移到条件块内**

当前 CMakeLists.txt 的结构：
```cmake
add_executable(${CMAKE_PROJECT_NAME})          # line 35

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)             # line 38

# ... link_directories ...

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE    # lines 46-48
    Core/Src/oled_display.c
)

# Add LVGL sources
file(GLOB_RECURSE LVGL_SOURCES                  # lines 51-54
    "Middlewares/Third_Party/LVGL/src/*.c"
    "Middlewares/Third_Party/LVGL/lv_port_disp_oled.c"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE    # lines 56-58
    ${LVGL_SOURCES}
)

# Add include paths
target_include_directories(... PRIVATE           # lines 61-65
    Core/Inc
    Middlewares/Third_Party/LVGL
    Middlewares/Third_Party/LVGL/src
)
```

替换为：

```cmake
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# ---- Display support (optional, default OFF) ----
option(ENABLE_DISPLAY "Enable OLED display + LVGL support" OFF)

if(ENABLE_DISPLAY)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_DISPLAY=1)

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        Core/Src/oled_display.c
    )

    file(GLOB_RECURSE LVGL_SOURCES
        "Middlewares/Third_Party/LVGL/src/*.c"
        "Middlewares/Third_Party/LVGL/lv_port_disp_oled.c"
    )

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${LVGL_SOURCES}
    )

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        Middlewares/Third_Party/LVGL
        Middlewares/Third_Party/LVGL/src
    )
endif()

# Add include paths (always needed)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
```

- [ ] **Step 2: 验证默认构建（无显示）编译通过**

```bash
cd /opt/coding/stm32/STM32F407-DISC-Audio
mkdir -p build && cd build
cmake .. && make
```
Expected: 编译通过，`oled_display.c` 和 LVGL 源码未被编译。

- [ ] **Step 3: 验证启用显示的构建编译通过**

```bash
cmake -DENABLE_DISPLAY=ON .. && make
```
Expected: 编译通过，行为和修改前完全一致。

- [ ] **Step 4: 提交**

```bash
git add CMakeLists.txt
git commit -m "build: add ENABLE_DISPLAY option (default OFF), conditionally compile OLED+LVGL"
```

---

### Task 2: 修改 main.h

**Files:**
- Modify: `Core/Inc/main.h:56` — 守卫 `UI_RequestAudioStatusRefresh` 声明
- Modify: `Core/Inc/main.h:127-128` — 守卫 `OLED_DC_Pin` 定义

- [ ] **Step 1: 守卫 `OLED_DC_Pin` 宏定义 (line 126-130)**

当前：
```c
/* USER CODE BEGIN Private defines */
#define OLED_DC_Pin GPIO_PIN_6
#define OLED_DC_GPIO_Port GPIOD

/* USER CODE END Private defines */
```

改为：
```c
/* USER CODE BEGIN Private defines */
#if ENABLE_DISPLAY
#define OLED_DC_Pin GPIO_PIN_6
#define OLED_DC_GPIO_Port GPIOD
#else
#define OLED_DC_Pin 0U  // 禁用显示时无害的零值，保证 CubeMX 代码正常编译
#define OLED_DC_GPIO_Port GPIOD
#endif

/* USER CODE END Private defines */
```

- [ ] **Step 2: 守卫 `UI_RequestAudioStatusRefresh` 声明 (line 55-57)**

当前：
```c
/* USER CODE BEGIN EFP */
void UI_RequestAudioStatusRefresh(void);

/* USER CODE END EFP */
```

改为：
```c
/* USER CODE BEGIN EFP */
#if ENABLE_DISPLAY
void UI_RequestAudioStatusRefresh(void);
#endif

/* USER CODE END EFP */
```

- [ ] **Step 3: 提交**

```bash
git add Core/Inc/main.h
git commit -m "main.h: guard OLED_DC_Pin and UI_RequestAudioStatusRefresh with ENABLE_DISPLAY"
```

---

### Task 3: 修改 main.c — 守卫显示相关代码

**Files:**
- Modify: `Core/Src/main.c`

这个任务涉及多处 `#if ENABLE_DISPLAY` 守卫，分布在 main.c 的不同位置。按从上到下的顺序逐步处理。

- [ ] **Step 1: 守卫 display 相关 include (lines 22, 28-30)**

当前：
```c
#include "main.h"
#include "cmsis_os.h"
#include "oled_display.h"
#include "usb_device.h"
#include "usbd_audio_if.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lvgl.h"
#include "lv_port_disp_oled.h"
#include "usbd_def.h"
/* USER CODE END Includes */
```

改为：

```c
#include "main.h"
#include "cmsis_os.h"
#if ENABLE_DISPLAY
#include "oled_display.h"
#endif
#include "usb_device.h"
#include "usbd_audio_if.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#if ENABLE_DISPLAY
#include "lvgl.h"
#include "lv_port_disp_oled.h"
#endif
#include "usbd_def.h"
/* USER CODE END Includes */
```

- [ ] **Step 2: 守卫 display 相关变量 (lines 57-67)**

当前：
```c
osThreadId defaultTaskHandle;
osThreadId audioTaskHandle;
osThreadId lcdTaskHandle;
osMessageQId uiEventQueueHandle;
/* USER CODE BEGIN PV */
static lv_obj_t *ui_usb_status_val = NULL;
static lv_obj_t *ui_playback_val = NULL;
static lv_obj_t *ui_codec_val = NULL;
static lv_obj_t *ui_sample_rate_val = NULL;
static lv_obj_t *ui_volume_val = NULL;
static lv_obj_t *ui_mute_val = NULL;
static lv_obj_t *ui_root_screen = NULL;
static uint32_t ui_last_audio_update_counter = 0U;

/* USER CODE END PV */
```

改为：

```c
osThreadId defaultTaskHandle;
osThreadId audioTaskHandle;
#if ENABLE_DISPLAY
osThreadId lcdTaskHandle;
osMessageQId uiEventQueueHandle;
#endif
/* USER CODE BEGIN PV */
#if ENABLE_DISPLAY
static lv_obj_t *ui_usb_status_val = NULL;
static lv_obj_t *ui_playback_val = NULL;
static lv_obj_t *ui_codec_val = NULL;
static lv_obj_t *ui_sample_rate_val = NULL;
static lv_obj_t *ui_volume_val = NULL;
static lv_obj_t *ui_mute_val = NULL;
static lv_obj_t *ui_root_screen = NULL;
static uint32_t ui_last_audio_update_counter = 0U;
#endif

/* USER CODE END PV */
```

- [ ] **Step 3: 守卫 `StartLcdTask` 声明 (line 80)**

当前：
```c
void StartDefaultTask(void const * argument);
void StartAudioTask(void const * argument);
void StartLcdTask(void const * argument);
```

改为：

```c
void StartDefaultTask(void const * argument);
void StartAudioTask(void const * argument);
#if ENABLE_DISPLAY
void StartLcdTask(void const * argument);
#endif
```

- [ ] **Step 4: 守卫 main() 中的 `MX_I2C2_Init()` 调用 (line 127)**

当前：
```c
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_I2S3_Init();
```

改为：
```c
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
#if ENABLE_DISPLAY
  MX_I2C2_Init();
#endif
  MX_I2S3_Init();
```

> 注意：`MX_I2C2_Init()` 函数体保留不删，`hi2c2` 变量也保留。禁用时函数不会被调用，链接器可移除。

- [ ] **Step 5: 守卫 UI 事件队列创建 (lines 145-148)**

当前：
```c
  /* USER CODE BEGIN RTOS_QUEUES */
  /* definition and creation of uiEventQueue */
  osMessageQDef(uiEventQueue, 8, uint16_t);
  uiEventQueueHandle = osMessageCreate(osMessageQ(uiEventQueue), NULL);
  /* USER CODE END RTOS_QUEUES */
```

改为：

```c
  /* USER CODE BEGIN RTOS_QUEUES */
#if ENABLE_DISPLAY
  /* definition and creation of uiEventQueue */
  osMessageQDef(uiEventQueue, 8, uint16_t);
  uiEventQueueHandle = osMessageCreate(osMessageQ(uiEventQueue), NULL);
#endif
  /* USER CODE END RTOS_QUEUES */
```

- [ ] **Step 6: 守卫 LCD 任务创建 (lines 160-162)**

当前：
```c
  /* definition and creation of lcdTask */
  osThreadDef(lcdTask, StartLcdTask, osPriorityNormal, 0, 2048);
  lcdTaskHandle = osThreadCreate(osThread(lcdTask), NULL);
```

改为：

```c
#if ENABLE_DISPLAY
  /* definition and creation of lcdTask */
  osThreadDef(lcdTask, StartLcdTask, osPriorityNormal, 0, 2048);
  lcdTaskHandle = osThreadCreate(osThread(lcdTask), NULL);
#endif
```

- [ ] **Step 7: 守卫所有 UI 函数定义 (lines 438-553)**

将 `/* USER CODE BEGIN 4 */` 到 `/* USER CODE END 4 */` 区域内的所有函数用 `#if ENABLE_DISPLAY` 包裹：

当前：
```c
/* USER CODE BEGIN 4 */
static uint32_t lvgl_tick_get_cb(void)
{
  ...
}

static lv_obj_t *create_info_row(...)
{
  ...
}

static void update_audio_info_ui(void)
{
  ...
}

void UI_RequestAudioStatusRefresh(void)
{
  ...
}

/* USER CODE END 4 */
```

改为：

```c
/* USER CODE BEGIN 4 */
#if ENABLE_DISPLAY
static uint32_t lvgl_tick_get_cb(void)
{
  return HAL_GetTick();
}

static lv_obj_t *create_info_row(lv_obj_t *parent, const char *name, lv_coord_t y)
{
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, name);
  lv_obj_set_size(label, 44, LV_SIZE_CONTENT);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 2, y);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_8, LV_PART_MAIN);

  lv_obj_t *value = lv_label_create(parent);
  lv_label_set_text(value, "--");
  lv_obj_set_size(value, 78, LV_SIZE_CONTENT);
  lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_align(value, LV_ALIGN_TOP_RIGHT, -2, y);
  lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(value, &lv_font_montserrat_8, LV_PART_MAIN);

  return value;
}

static void update_audio_info_ui(void)
{
  const char *usb_status_text = "Detached";
  const char *playback_text = "Idle";

  switch (hUsbDeviceFS.dev_state)
  {
    case USBD_STATE_CONFIGURED:
      usb_status_text = "Configured";
      break;
    case USBD_STATE_ADDRESSED:
      usb_status_text = "Addressed";
      break;
    case USBD_STATE_SUSPENDED:
      usb_status_text = "Suspended";
      break;
    case USBD_STATE_DEFAULT:
    default:
      usb_status_text = "Detached";
      break;
  }

  switch (AUDIO_UI_GetPlaybackState())
  {
    case UI_PLAYBACK_STATE_ACTIVE:
      playback_text = "Active";
      break;
    case UI_PLAYBACK_STATE_STARTING:
      playback_text = "Starting";
      break;
    case UI_PLAYBACK_STATE_IDLE:
    default:
      playback_text = "Idle";
      break;
  }

  if (ui_usb_status_val != NULL)
    lv_label_set_text(ui_usb_status_val, usb_status_text);
  if (ui_playback_val != NULL)
    lv_label_set_text(ui_playback_val, playback_text);
  if (ui_codec_val != NULL)
    lv_label_set_text(ui_codec_val,
                      (AUDIO_UI_GetCodecReady() != 0U) ? "Ready" : "Off");
  if (ui_sample_rate_val != NULL)
    lv_label_set_text_fmt(ui_sample_rate_val, "%lu Hz",
                          (unsigned long)AUDIO_UI_GetSampleRate());
  if (ui_volume_val != NULL)
    lv_label_set_text_fmt(ui_volume_val, "%u%%", AUDIO_UI_GetVolume());
  if (ui_mute_val != NULL)
    lv_label_set_text(ui_mute_val,
                      (AUDIO_UI_GetMuted() != 0U) ? "Muted" : "Off");
  if (ui_root_screen != NULL)
    lv_obj_invalidate(ui_root_screen);
}

void UI_RequestAudioStatusRefresh(void)
{
  if (uiEventQueueHandle != NULL)
    (void)osMessagePut(uiEventQueueHandle, UI_EVENT_AUDIO_REFRESH, 0U);
}
#endif

/* USER CODE END 4 */
```

注意：`UI_PLAYBACK_STATE_*` 和 `UI_EVENT_AUDIO_REFRESH` 宏定义（lines 90-93）留在 `/* USER CODE BEGIN 0 */` 中不守卫，它们是简单的常量定义，不影响功能。

- [ ] **Step 8: 守卫 `create_audio_info_ui` 函数 (lines 561-593)**

当前：
```c
static void create_audio_info_ui(void)
{
  /* Get active screen */
  lv_obj_t *scr = lv_scr_act();
  ...
}
```

改为：

```c
#if ENABLE_DISPLAY
static void create_audio_info_ui(void)
{
  /* Get active screen */
  lv_obj_t *scr = lv_scr_act();
  ui_root_screen = scr;

  /* Set background color to black */
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "USB Audio");
  lv_obj_set_width(title, 124);
  lv_label_set_long_mode(title, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_8, LV_PART_MAIN);

  lv_obj_t *line1 = lv_obj_create(scr);
  lv_obj_set_size(line1, 124, 1);
  lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 12);
  lv_obj_set_style_bg_color(line1, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_border_width(line1, 0, LV_PART_MAIN);

  ui_usb_status_val = create_info_row(scr, "LINK", 16);
  ui_playback_val = create_info_row(scr, "PLAY", 25);
  ui_codec_val = create_info_row(scr, "CODEC", 34);
  ui_sample_rate_val = create_info_row(scr, "RATE", 43);
  ui_volume_val = create_info_row(scr, "VOL", 52);
  ui_mute_val = NULL;

  update_audio_info_ui();
}
#endif
```

- [ ] **Step 9: 守卫 `StartLcdTask` 函数定义 (lines 629-667)**

当前：
```c
void StartLcdTask(void const * argument)
{
  /* USER CODE BEGIN StartLcdTask */
  ...
  /* USER CODE END StartLcdTask */
}
```

改为：

```c
#if ENABLE_DISPLAY
void StartLcdTask(void const * argument)
{
  uint32_t ui_refresh_tick = 0U;
  osEvent ui_event;

  /* USER CODE BEGIN StartLcdTask */
  UNUSED(argument);

  /* Initialize LVGL and the display */
  lv_init();
  lv_tick_set_cb(lvgl_tick_get_cb);
  lv_port_disp_init();

  /* Create USB Audio Information UI */
  create_audio_info_ui();

  /* Main loop: handle LVGL tasks with sufficient delay to avoid blocking audio */
  for(;;)
  {
    ui_event = osMessageGet(uiEventQueueHandle, 30U);

    if (ui_event.status == osEventMessage)
    {
      update_audio_info_ui();
      ui_last_audio_update_counter = AUDIO_UI_GetUpdateCounter();
      ui_refresh_tick = HAL_GetTick();
    }
    else if ((AUDIO_UI_GetUpdateCounter() != ui_last_audio_update_counter) ||
             ((HAL_GetTick() - ui_refresh_tick) >= 500U))
    {
      update_audio_info_ui();
      ui_last_audio_update_counter = AUDIO_UI_GetUpdateCounter();
      ui_refresh_tick = HAL_GetTick();
    }

    lv_timer_handler();
  }
  /* USER CODE END StartLcdTask */
}
#endif
```

注意：函数内原有的 `/* USER CODE BEGIN/END StartLcdTask */` 标记保留，所以守卫放在 `void StartLcdTask(...)` 那一行之前和函数结束 `}` 之后。

- [ ] **Step 10: 验证默认构建（无显示）编译通过**

```bash
cd build && cmake .. && make
```
Expected: 编译通过，无 linker error。

- [ ] **Step 11: 验证启用显示构建编译通过**

```bash
cmake -DENABLE_DISPLAY=ON .. && make
```
Expected: 编译通过，行为与修改前一致。

- [ ] **Step 12: 提交**

```bash
git add Core/Src/main.c
git commit -m "main.c: guard all display code with ENABLE_DISPLAY macro"
```

---

### Task 4: 修改 usbd_audio_if.c — 守卫 UI_RequestAudioStatusRefresh 调用

**Files:**
- Modify: `USB_DEVICE/App/usbd_audio_if.c:525-529`

- [ ] **Step 1: 守卫 `AUDIO_UI_NotifyChanged` 中的 `UI_RequestAudioStatusRefresh()` 调用 (line 528)**

当前：
```c
static void AUDIO_UI_NotifyChanged(void)
{
  audio_ui_update_counter++;
  UI_RequestAudioStatusRefresh();
}
```

改为：

```c
static void AUDIO_UI_NotifyChanged(void)
{
  audio_ui_update_counter++;
#if ENABLE_DISPLAY
  UI_RequestAudioStatusRefresh();
#endif
}
```

- [ ] **Step 2: 验证默认构建（无显示）编译通过**

```bash
cd build && cmake .. && make
```
Expected: 编译通过。

- [ ] **Step 3: 提交**

```bash
git add USB_DEVICE/App/usbd_audio_if.c
git commit -m "usbd_audio_if: guard UI_RequestAudioStatusRefresh call with ENABLE_DISPLAY"
```

---

### Task 5: 最终验证

- [ ] **Step 1: 验证默认构建（ENABLE_DISPLAY=OFF）编译成功**

```bash
cd /opt/coding/stm32/STM32F407-DISC-Audio/build
cmake .. && make
```
Expected: 编译通过，二进制不包含 LVGL/OLED 代码。

- [ ] **Step 2: 验证启用构建（ENABLE_DISPLAY=ON）编译成功**

```bash
cmake -DENABLE_DISPLAY=ON .. && make
```
Expected: 编译通过，行为和修改前完全一致。

- [ ] **Step 3: 对比两种构建的 binary 大小，确认显示关闭时固件明显变小**

```bash
# 先构建无显示版本
cmake .. && make
arm-none-eabi-size build/STM32F407-DISC-Audio.elf  # 或其他输出路径

# 再构建有显示版本
cmake -DENABLE_DISPLAY=ON .. && make
arm-none-eabi-size build/STM32F407-DISC-Audio.elf
```
Expected: 无显示版本固件明显更小（无 LVGL 库代码，约节省 50KB+ Flash）。
```
