# 显示模块可选化设计

## 目标

将 STM32F407-DISC-Audio 项目的显示部分（OLED + LVGL）改为可选模块，默认关闭。关闭时不编译显示源码、不初始化显示外设、不创建显示任务。

## 方案

CMake option `ENABLE_DISPLAY` (默认 OFF)，传递编译宏 `ENABLE_DISPLAY=1`。所有显示相关代码用 `#if ENABLE_DISPLAY` 条件编译。

开启方式：`cmake -DENABLE_DISPLAY=ON`

## 影响范围

### CMakeLists.txt
- 新增 `option(ENABLE_DISPLAY ...)`
- `oled_display.c`、LVGL 源码、LVGL include 路径移到 `if(ENABLE_DISPLAY)` 块内
- `target_compile_definitions` 条件添加 `ENABLE_DISPLAY=1`

### main.h
- `OLED_DC_Pin` 宏定义
- `UI_RequestAudioStatusRefresh()` 声明
以上用 `#if ENABLE_DISPLAY` 守卫

### main.c（改动集中在此）
需守卫的代码块：

1. **Include**: `oled_display.h`、`lvgl.h`、`lv_port_disp_oled.h`
2. **变量**: `hi2c2`、`lcdTaskHandle`、`uiEventQueueHandle`、LVGL UI 静态变量
3. **外设初始化**: `MX_I2C2_Init()` 调用、OLED GPIO (`OLED_RES`/`OLED_CS`/`OLED_DC`) 初始化代码
4. **RTOS 对象**: UI 事件队列创建、LCD 任务创建
5. **函数**: `lvgl_tick_get_cb`、`create_info_row`、`update_audio_info_ui`、`create_audio_info_ui`、`StartLcdTask`、`UI_RequestAudioStatusRefresh`

注意：所有条件编译指令放在 `USER CODE BEGIN/END` 区域内，不影响 CubeMX 重新生成。

### usbd_audio_if.c
- `AUDIO_UI_NotifyChanged()` 中 `UI_RequestAudioStatusRefresh()` 调用用 `#if ENABLE_DISPLAY` 守卫
- `AUDIO_UI_Get*` 函数保持不变（音频模块自身仍需这些状态变量）

## 不修改的文件
- `oled_display.c/h` — 不编译即无影响
- `lv_port_disp_oled.c/h` — 同上
- `st7735_lcd.c/h` — 当前未使用
- `lv_port_disp_st7735s.c/h` — 当前未使用

## 验证方式
- 默认构建（无 `-DENABLE_DISPLAY`）：编译通过，固件不包含 OLED/LVGL 代码
- `-DENABLE_DISPLAY=ON` 构建：行为与当前完全一致
