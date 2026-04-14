# STM32F407-DISC-Audio

这是一个基于 `STM32F407 Discovery` 的 USB Audio + LCD 状态显示项目。

当前项目的主要目标是：

- 将开发板做成一个 USB Audio Class 输出设备
- 通过板载 `CS43L22` 编解码器输出音频
- 使用 `1.8 寸 ST7735S LCD` 显示当前音频运行状态
- 为后续功能扩展和二次开发保留清晰的工程结构

这份 README 尽量按“接手文档”的标准来写，重点说明：

- 依赖环境
- 编译方式
- 调试方式
- I/O 引脚分配
- 使用的系统和中间件
- RTOS 任务划分
- 音频与显示的实现结构
- 当前已经实现的功能
- 后续二次开发时需要注意的点

## 1. 项目概述

当前已经实现的主要功能：

- USB FS 音频播放设备（USB Audio Class 1.0）
- 音频通过板载 `CS43L22` 输出
- `I2S3 + DMA` 向 codec 持续送音频数据
- `128 x 160` 的 ST7735S LCD 实时显示音频状态
- 基于 LVGL 的轻量级状态页 UI
- 支持 macOS 侧音量与静音控制
- 支持暂停/恢复播放
- 支持 USB 拔插后的重新工作

当前屏幕会显示这些信息：

- USB 链路状态
- 播放状态
- Codec 状态
- 采样率
- 音量
- 静音状态

## 2. 硬件平台

主控板：

- `STM32F407VG`，STM32F4 Discovery 系列开发板

主要外设：

- 板载音频 Codec：`CS43L22`
- USB FS 设备接口
- 1.8 寸 TFT LCD，控制器：`ST7735S`

显示屏参数：

- 分辨率：`128 x 160`
- 当前按竖屏方式使用

## 3. 软件架构

项目主要由以下几层组成：

- STM32CubeMX 生成的基础工程
- STM32 HAL
- FreeRTOS（通过 CMSIS-RTOS 封装）
- STM32 USB Device Library
- USB Audio Class
- LVGL v9
- 自定义 ST7735S bit-bang 显示驱动

关键源码位置：

- 入口、任务、UI：
  - [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- 全局引脚定义：
  - [main.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/main.h)
- USB Audio 接口层：
  - [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
  - [usbd_audio_if.h](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.h)
- USB 设备初始化：
  - [usb_device.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usb_device.c)
- USB Audio 类核心：
  - [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)
- ST7735S 驱动：
  - [st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
  - [st7735_lcd.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/st7735_lcd.h)
- LVGL 显示适配层：
  - [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)
- FreeRTOS 配置：
  - [FreeRTOSConfig.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/FreeRTOSConfig.h)
- LVGL 配置：
  - [lv_conf.h](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_conf.h)

## 4. 依赖环境

当前工程使用的开发环境大致如下：

- CMake
- Ninja
- GNU Arm Embedded Toolchain
- STM32CubeCLT
- ST-LINK GDB Server
- VS Code
- Cortex-Debug 插件

从当前调试配置中可以看到的本地工具路径：

- GDB：
  - `/opt/ST/STM32CubeCLT_1.21.0/GNU-tools-for-STM32/bin/arm-none-eabi-gdb`
- ST-LINK GDB Server：
  - `/opt/ST/STM32CubeCLT_1.21.0/STLink-gdb-server/bin/ST-LINK_gdbserver`
- STM32CubeProgrammer：
  - `/opt/ST/STM32CubeCLT_1.21.0/STM32CubeProgrammer/bin`
- SVD：
  - `/opt/ST/STM32CubeCLT_1.21.0/STMicroelectronics_CMSIS_SVD/STM32F407.svd`

相关配置文件：

- [CMakePresets.json](/opt/coding/stm32/STM32F407-DISC-Audio/CMakePresets.json)
- [.vscode/launch.json](/opt/coding/stm32/STM32F407-DISC-Audio/.vscode/launch.json)
- [gcc-arm-none-eabi.cmake](/opt/coding/stm32/STM32F407-DISC-Audio/cmake/gcc-arm-none-eabi.cmake)

## 5. 编译

工程名：

- `STM32F407-DISC-Audio`

构建系统：

- `CMake + Ninja`

主 CMake 文件：

- [CMakeLists.txt](/opt/coding/stm32/STM32F407-DISC-Audio/CMakeLists.txt)

当前可用的 Preset：

- `Debug`
- `Release`

典型编译流程：

```bash
cmake --preset Debug
cmake --build build/Debug
```

Release 版本：

```bash
cmake --preset Release
cmake --build build/Release
```

调试版本默认输出：

- `build/Debug/STM32F407-DISC-Audio.elf`

说明：

- LVGL 源码通过递归方式加入编译
- 自定义显示适配层也已加入编译
- STM32CubeMX 生成的基础部分通过：
  - `add_subdirectory(cmake/stm32cubemx)`
  引入

## 6. 调试

当前已配置 VS Code 调试。

可用调试项：

- `Debug STM32F407-DISC-Audio (ST-LINK)`
- `Attach STM32F407-DISC-Audio (ST-LINK)`

调试配置文件：

- [.vscode/launch.json](/opt/coding/stm32/STM32F407-DISC-Audio/.vscode/launch.json)

关键调试参数：

- 设备：`STM32F407VG`
- 接口：`SWD`
- 服务器：`stlink`
- `runToEntryPoint` 当前注释掉了，因此不会强制停在 `main`

建议重点打断点的位置：

- 音频状态机：
  - [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
- LCD 任务和 UI：
  - [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- USB Audio 数据包处理：
  - [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

### 栈溢出检测

当前已开启：

- `configCHECK_FOR_STACK_OVERFLOW = 2`
  - [FreeRTOSConfig.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/FreeRTOSConfig.h)

对应处理函数：

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
  - `vApplicationStackOverflowHook(...)`

当前行为：

- 关闭中断
- `LD3` 持续闪烁

这个机制是后来专门为排查 LVGL + LCD 刷新时的栈问题加上的。

## 7. 所用系统与中间件

### MCU / HAL

- STM32F4 HAL
- `I2C1` 用于控制音频 Codec
- `I2S3` 用于向 Codec 发送音频数据
- `DMA1_Stream5` 用于 I2S TX DMA

### RTOS

- FreeRTOS `V10.3.1`
- CMSIS-RTOS 封装

当前关键配置：

- Tick：`1000 Hz`
- Heap：`15360`
- 最大优先级数：`7`

### USB

- STM32 USB Device Library
- USB Audio Class 1.0
- USB Full-Speed Device

### 图形界面

- LVGL v9
- 自定义 ST7735S 显示驱动
- partial refresh 模式

## 8. I/O 设置

### USB FS

- `PA11` -> `OTG_FS_DM`
- `PA12` -> `OTG_FS_DP`
- `PA9`  -> `VBUS_FS`
- `PA10` -> `OTG_FS_ID`

### 音频 Codec：CS43L22（I2C1）

- `PB6` -> `Audio_SCL`
- `PB9` -> `Audio_SDA`
- `PD4` -> `Audio_RST`

### 音频数据输出：I2S3

- `PA4`  -> `I2S3_WS`
- `PC7`  -> `I2S3_MCK`
- `PC10` -> `I2S3_SCK`
- `PC12` -> `I2S3_SD`

当前 I2S 配置：

- 模式：`I2S_MODE_MASTER_TX`
- 标准：`I2S_STANDARD_PHILIPS`
- 数据位宽：`16-bit`
- MCLK：开启
- 采样率：`48 kHz`

### LCD：ST7735S

这块 LCD 目前用 GPIO 模拟串行接口，不是硬件 SPI，也不是 I2C。

引脚映射如下：

- `PD11` -> `LCD_RST`
- `PD10` -> `LCD_DC`
- `PA3`  -> `LCD_CS`
- `PD9`  -> `LCD_BLK`
- `PA1`  -> `LCD_SCL`
- `PA2`  -> `LCD_SDA`

特别注意：

- `LCD_SCL` / `LCD_SDA` 是显示驱动用的软件时序引脚
- 这两根线不是 I2C

### 板载 LED

- `PD12` -> `LD4`
- `PD13` -> `LD3`
- `PD14` -> `LD5`
- `PD15` -> `LD6`

## 9. RTOS 任务划分

定义位置：

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)

### `defaultTask`

- 入口：`StartDefaultTask`
- 优先级：`osPriorityNormal`
- 栈：`512`

职责：

- 初始化 USB 设备栈
- 调用 `MX_USB_DEVICE_Init()`

### `audioTask`

- 入口：`StartAudioTask`
- 优先级：`osPriorityHigh`
- 栈：`512`

职责：

- 每 `1 ms` 调用一次 `AUDIO_ServiceTaskStep()`
- 在任务上下文里处理音频启动/停止/初始化/反初始化
- 避免在 USB 回调里做过重操作

### `lcdTask`

- 入口：`StartLcdTask`
- 优先级：`osPriorityNormal`
- 栈：`2048`

职责：

- 初始化 LVGL
- 绑定 LVGL tick 到 `HAL_GetTick()`
- 初始化 ST7735S 显示端口
- 创建状态页 UI
- 处理 UI 刷新消息
- 调用 `lv_timer_handler()`

### 消息队列

当前用于 UI 刷新的队列：

- `uiEventQueue`

作用：

- 音频侧状态变化时发消息
- `lcdTask` 收到消息后刷新状态页

## 10. 音频系统说明

主要实现文件：

- [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
- [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

### 音频主流程

1. 主机枚举 USB Audio 设备
2. USB OUT 音频包进入 USB ring buffer
3. USB 类驱动发出 `AUDIO_CMD_START`
4. `audioTask` 负责初始化 codec
5. 等 ring buffer 有足够数据后，再启动 DMA
6. `I2S3 + DMA` 持续把数据送到 `CS43L22`

### 为什么音频启动要放在 `audioTask`

这个项目里有意识地把 codec 初始化、DMA 启动、停止等动作从 USB 回调里挪到了任务上下文。原因是：

- codec 初始化包含 I2C 写寄存器和延时
- 如果放在 USB 回调路径里，容易造成时序不稳
- 放到 `audioTask` 后，启动链路更稳，后续维护也更清晰

### 固定采样率

当前采样率固定为：

- `48 kHz`

UI 里通过：

- `AUDIO_UI_GetSampleRate()`
  返回

### 暂停 / 恢复 / 拔插

当前已经实现：

- 暂停后可恢复播放
- USB 拔掉后可重新连接恢复
- stop 时清理 ring buffer
- stop 时复位 USB Audio 内部流状态

这部分之所以重要，是因为 ST 的 USB Audio 类内部只有在 ring buffer 回到初始状态时，才会重新触发 `AUDIO_CMD_START`。

### 音量 / 静音

当前设计如下：

- 板端最大输出限制为 `95%`
- 电脑侧音量会线性映射到 `0..95%`
- 暂停 / 恢复时，会记住暂停前音量
- 静音状态拆成了：
  - 用户静音请求
  - 实际输出静音状态

这样可以避免 pause/stop/restart 时 UI 与实际音频状态互相污染。

## 11. 显示系统说明

主要文件：

- [st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
- [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)
- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)

### 显示参数

- 控制器：`ST7735S`
- 分辨率：`128 x 160`
- 颜色格式：`RGB565`
- 当前按竖屏使用

### LVGL 显示模式

当前显示缓冲配置在：

- [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)

使用的是 partial buffer 模式。

### 为什么 `ST7735_FillRect()` 改成逐行写

之前在整屏刷新时，显示会出现“整屏变斜”的问题。最后确认更稳的方案是：

- 每一行单独设置窗口
- 每一行单独写像素

虽然这样刷新速度会慢一些，但稳定性明显更好，尤其是当前 LCD 还是 bit-bang 驱动。

## 12. LVGL 配置说明

配置文件：

- [lv_conf.h](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_conf.h)

当前关键设置：

- 横向分辨率：`128`
- 纵向分辨率：`160`
- 色深：`16-bit`

已启用字体：

- `Montserrat 8`
- `Montserrat 10`
- `Montserrat 12`
- `Montserrat 14`

当前 UI 特点：

- 黑底
- 小型状态面板
- 通过消息通知触发刷新

## 13. 当前功能输出

目前项目可输出的功能有：

- 在电脑上识别为 USB Audio 输出设备
- 通过板载 codec 输出立体声音频
- 在 LCD 上显示当前音频状态

当前不包含的功能：

- USB Audio 输入（麦克风）
- 多采样率动态切换
- 多页面 UI
- 触摸或按键交互

## 14. 重要实现决策

### 1. LCD 当前不是硬件 SPI

LCD 目前使用 GPIO bit-bang。这样做的好处是接线灵活、调试直观，但性能不是最优。

如果未来 UI 更复杂，建议优先考虑迁移到硬件 SPI。

### 2. 音频状态机不要轻易搬回 USB 回调

当前音频的稳定性，和“重操作在 `audioTask` 中执行”这一设计关系很大。除非整体重构，否则不建议再把 codec init / DMA start 放回 USB 回调。

### 3. 栈大小已经不是 CubeMX 默认值

曾经显示异常的一个重要原因是任务栈太小。现在 `lcdTask` 的栈已经增大，后续修改时不要随意改回默认小值。

### 4. UI 更新应尽量留在 LCD 任务上下文

当前音频侧只负责发刷新请求，真正的 LVGL 对象更新在 `lcdTask` 中完成。这个边界是有意义的，建议保持。

## 15. 二次开发最常改的文件

### 音频相关

- [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
- [usbd_audio_if.h](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.h)
- [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

### 显示 / UI 相关

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- [st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
- [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)
- [lv_conf.h](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_conf.h)

### 系统 / RTOS 相关

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- [FreeRTOSConfig.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/FreeRTOSConfig.h)

## 16. 后续建议改进方向

建议优先考虑的后续工作：

- 将 LCD 从 bit-bang 迁移到硬件 SPI
- 清理 LVGL 配置中的版本兼容问题
- 如果未来支持多采样率，补齐主机实际采样率显示
- 增加更清晰的音频调试日志或状态灯
- 将 UI 数据模型进一步抽象
- 补充系统负载和 DMA 时序评估

## 17. 新接手开发者建议阅读顺序

如果是第一次接手这个工程，建议按下面顺序阅读：

1. [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
2. [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
3. [st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
4. [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

这样比较容易建立正确的心智模型：

- 先看任务和系统框架
- 再看音频控制逻辑
- 再看显示底层驱动
- 最后看 USB Audio 中间件细节

