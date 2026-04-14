# STM32F407-DISC-Audio

STM32F407 Discovery board based USB Audio + LCD status display project.

This project turns the STM32F407 Discovery into a USB Audio Class output device and shows runtime status on a 1.8-inch ST7735S LCD using LVGL.

The repository is intended to be practical for follow-up development, debugging, and feature expansion. This README focuses on the current architecture, build/debug flow, I/O mapping, task split, and important implementation details that matter when modifying the project.

## 1. Project Overview

Main functions currently implemented:

- USB Audio Class 1.0 playback device over USB FS
- Audio output through on-board `CS43L22` codec
- Audio transport to codec via `I2S3 + DMA`
- Runtime audio status display on `128 x 160` ST7735S LCD
- LVGL-based compact status UI
- Volume and mute control from macOS
- Recovery logic for pause/resume and USB unplug/replug

Current screen shows:

- USB link state
- Playback state
- Codec state
- Sample rate
- Volume
- Mute state

## 2. Hardware Platform

Primary board:

- `STM32F407VG` based STM32F4 Discovery

Main external/peripheral blocks:

- On-board audio codec: `CS43L22`
- USB FS device interface
- ST7735S LCD module, `128 x 160`, portrait layout

## 3. Software Stack

The project is built from these main layers:

- STM32CubeMX generated base project
- HAL drivers
- FreeRTOS via CMSIS-RTOS wrapper
- STM32 USB Device Library, Audio Class
- LVGL v9 display layer
- Custom ST7735S bit-bang LCD driver

Important source areas:

- Application entry and RTOS tasks:
  - [Core/Src/main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- Global pin definitions:
  - [Core/Inc/main.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/main.h)
- Audio codec / playback control:
  - [USB_DEVICE/App/usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
  - [USB_DEVICE/App/usbd_audio_if.h](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.h)
- USB device startup:
  - [USB_DEVICE/App/usb_device.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usb_device.c)
- USB Audio class core:
  - [Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)
- LCD low-level driver:
  - [Core/Src/st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
  - [Core/Inc/st7735_lcd.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/st7735_lcd.h)
- LVGL display port:
  - [Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)
- RTOS configuration:
  - [Core/Inc/FreeRTOSConfig.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/FreeRTOSConfig.h)
- LVGL configuration:
  - [Middlewares/Third_Party/LVGL/lv_conf.h](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_conf.h)

## 4. Development Environment

The project is currently set up around the following toolchain and workflow:

- CMake
- Ninja
- GNU Arm Embedded Toolchain
- STM32CubeCLT
- ST-LINK GDB Server
- VS Code + Cortex-Debug

Observed local tool paths from current debug configuration:

- GDB:
  - `/opt/ST/STM32CubeCLT_1.21.0/GNU-tools-for-STM32/bin/arm-none-eabi-gdb`
- ST-LINK GDB Server:
  - `/opt/ST/STM32CubeCLT_1.21.0/STLink-gdb-server/bin/ST-LINK_gdbserver`
- STM32CubeProgrammer:
  - `/opt/ST/STM32CubeCLT_1.21.0/STM32CubeProgrammer/bin`
- SVD:
  - `/opt/ST/STM32CubeCLT_1.21.0/STMicroelectronics_CMSIS_SVD/STM32F407.svd`

Relevant config files:

- [CMakePresets.json](/opt/coding/stm32/STM32F407-DISC-Audio/CMakePresets.json)
- [.vscode/launch.json](/opt/coding/stm32/STM32F407-DISC-Audio/.vscode/launch.json)
- [cmake/gcc-arm-none-eabi.cmake](/opt/coding/stm32/STM32F407-DISC-Audio/cmake/gcc-arm-none-eabi.cmake)

## 5. Build

Project name:

- `STM32F407-DISC-Audio`

Build system:

- CMake + Ninja

Main CMake entry:

- [CMakeLists.txt](/opt/coding/stm32/STM32F407-DISC-Audio/CMakeLists.txt)

Configured presets:

- `Debug`
- `Release`

Typical build flow:

```bash
cmake --preset Debug
cmake --build build/Debug
```

Or for Release:

```bash
cmake --preset Release
cmake --build build/Release
```

Expected ELF path for debug:

- `build/Debug/STM32F407-DISC-Audio.elf`

Notes:

- LVGL sources are added recursively from `Middlewares/Third_Party/LVGL/src`
- Custom display port is also added explicitly
- STM32CubeMX generated sources are brought in via:
  - `add_subdirectory(cmake/stm32cubemx)`

## 6. Debug

VS Code debug launch is already configured.

Available configurations:

- `Debug STM32F407-DISC-Audio (ST-LINK)`
- `Attach STM32F407-DISC-Audio (ST-LINK)`

Debug config file:

- [.vscode/launch.json](/opt/coding/stm32/STM32F407-DISC-Audio/.vscode/launch.json)

Key debug details:

- Device: `STM32F407VG`
- Interface: `SWD`
- Server type: `stlink`
- `runToEntryPoint` is currently commented out, so launch does not forcibly stop at `main`

Useful runtime debug locations:

- Audio start/stop state machine:
  - [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
- LCD task and LVGL UI:
  - [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- USB class audio packet handling:
  - [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

### Stack Overflow Detection

Stack overflow detection is enabled:

- `configCHECK_FOR_STACK_OVERFLOW = 2`
  - [FreeRTOSConfig.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/FreeRTOSConfig.h)

Overflow hook implementation:

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
  - `vApplicationStackOverflowHook(...)`

Current behavior on stack overflow:

- interrupts disabled
- `LD3` toggles in a loop

This was added because LVGL + display refresh caused unstable behavior with too-small task stacks.

## 7. Systems Used

### MCU / HAL

- STM32F4 HAL
- `I2C1` for audio codec control
- `SPI3/I2S3` in `I2S master TX` mode for audio output
- `DMA1_Stream5` for I2S TX DMA

### RTOS

- FreeRTOS `V10.3.1`
- CMSIS-RTOS wrapper

Relevant config:

- Tick rate: `1000 Hz`
- Heap size: `15360`
- Max priorities: `7`

### USB

- STM32 USB Device Library
- USB Audio Class 1.0
- Full-speed USB device

### Graphics

- LVGL v9
- Custom ST7735S display driver
- Partial refresh mode

## 8. Current RTOS Task Layout

Defined in:

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)

### `defaultTask`

- Entry: `StartDefaultTask`
- Priority: `osPriorityNormal`
- Stack: `512`

Responsibilities:

- initialize USB device stack via `MX_USB_DEVICE_Init()`

### `audioTask`

- Entry: `StartAudioTask`
- Priority: `osPriorityHigh`
- Stack: `512`

Responsibilities:

- run `AUDIO_ServiceTaskStep()` every 1 ms
- perform audio state machine work outside USB callback context
- handle start/stop/init/deinit sequencing for codec and DMA

### `lcdTask`

- Entry: `StartLcdTask`
- Priority: `osPriorityNormal`
- Stack: `2048`

Responsibilities:

- initialize LVGL
- bind LVGL tick callback to `HAL_GetTick()`
- initialize ST7735S display port
- create the audio status page
- process UI refresh events via message queue
- run `lv_timer_handler()`

### Message Queue

Queue used by display update path:

- `uiEventQueue`

Purpose:

- audio side posts status refresh events
- LCD task wakes and updates the visible LVGL labels

## 9. I/O Mapping

### USB FS

- `PA11` -> `OTG_FS_DM`
- `PA12` -> `OTG_FS_DP`
- `PA9`  -> `VBUS_FS`
- `PA10` -> `OTG_FS_ID`

### Audio Codec Control (`CS43L22`) via I2C1

- `PB6` -> `Audio_SCL`
- `PB9` -> `Audio_SDA`
- `PD4` -> `Audio_RST`

### Audio Data Path via I2S3

- `PA4`  -> `I2S3_WS`
- `PC7`  -> `I2S3_MCK`
- `PC10` -> `I2S3_SCK`
- `PC12` -> `I2S3_SD`

Configuration summary:

- mode: `I2S_MODE_MASTER_TX`
- standard: `I2S_STANDARD_PHILIPS`
- data format: `16-bit`
- MCLK: enabled
- sample rate: `48 kHz`

### LCD: ST7735S

The LCD uses bit-bang serial GPIO. `LCD_SCL` and `LCD_SDA` are not I2C.

Pin mapping:

- `PD11` -> `LCD_RST`
- `PD10` -> `LCD_DC`
- `PA3`  -> `LCD_CS`
- `PD9`  -> `LCD_BLK`
- `PA1`  -> `LCD_SCL`
- `PA2`  -> `LCD_SDA`

Important note:

- `LCD_SCL` / `LCD_SDA` are software-driven display serial pins
- they are not connected to STM32 I2C peripheral logic

### LEDs

- `PD12` -> `LD4`
- `PD13` -> `LD3`
- `PD14` -> `LD5`
- `PD15` -> `LD6`

## 10. Audio Pipeline

Main implementation:

- [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
- [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

### Flow

1. Host enumerates the device as USB Audio
2. USB OUT packets are received into the STM32 USB audio ring buffer
3. USB class code triggers `AUDIO_CMD_START`
4. Audio task initializes codec if needed
5. Playback DMA starts only after enough USB buffer data has accumulated
6. I2S DMA streams PCM data to the `CS43L22`

### Why audio startup is handled in `audioTask`

Heavy operations such as codec init were intentionally moved out of USB callback timing. This avoids unstable behavior caused by doing I2C resets, delays, and codec programming directly inside USB event context.

### Sample Rate

Current sample rate is fixed:

- `48 kHz`

Reported to UI via:

- `AUDIO_UI_GetSampleRate()`

### Pause / Resume / Replug

There is explicit logic for:

- stopping DMA and codec cleanly
- clearing the USB ring buffer
- resetting USB Audio internal stream state
- allowing playback to restart after pause or unplug/replug

This was necessary because the USB Audio class middleware only reissues `AUDIO_CMD_START` when its internal ring state returns to the initial condition.

### Volume / Mute

Host volume is handled through USB Audio feature unit controls.

Current design:

- board output is capped at `95%`
- host volume is linearly mapped into `0..95%`
- pause/resume preserves current volume instead of forcing max volume on restart

Mute handling is split into:

- user mute request
- actual current codec output mute state

This separation avoids UI and audio state confusion during pause/stop/restart.

## 11. Display Pipeline

Main files:

- [st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
- [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)
- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)

### Display Characteristics

- controller: `ST7735S`
- resolution: `128 x 160`
- portrait orientation
- RGB565 color

### LVGL Display Mode

Current display buffer:

- half-screen style partial buffer

Configured in:

- [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)

### Important Driver Detail

`ST7735_FillRect()` was changed to write row-by-row instead of writing one long uninterrupted block for the entire update region.

Reason:

- full-screen long serial transfers were causing the display image to become diagonally skewed
- per-row windowing greatly improved stability on this bit-bang interface

Tradeoff:

- slower refresh
- much more stable rendering

## 12. LVGL Configuration Notes

Config file:

- [lv_conf.h](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_conf.h)

Important current settings:

- resolution:
  - `LV_HOR_RES_MAX = 128`
  - `LV_VER_RES_MAX = 160`
- color depth:
  - `LV_COLOR_DEPTH = 16`
- fonts enabled:
  - `Montserrat 8`
  - `Montserrat 10`
  - `Montserrat 12`
  - `Montserrat 14`

Current UI behavior:

- black background
- compact status labels
- runtime update via queue-triggered LCD task refresh

## 13. Important Known Design Decisions

### 1. LCD bit-bang instead of hardware SPI

The ST7735S is currently driven by GPIO bit-bang. This made bring-up easier with the existing pin assignment, but it is not the highest performance option.

If future UI complexity increases, migrating the display to hardware SPI is a strong improvement candidate.

### 2. Audio task separation is intentional

Audio startup and shutdown are not executed directly in USB callbacks. This is deliberate and should be preserved unless the whole timing model is redesigned.

### 3. Task stack sizes matter

Display instability was previously caused by undersized task stacks. Current stack sizes are intentionally larger than the CubeMX defaults.

### 4. UI updates should remain in LCD context

Audio code only posts refresh requests. Actual LVGL object updates are performed in the LCD task.

## 14. Basic Functional Outputs

What the current firmware outputs:

- USB Audio device on the host computer
- stereo audio through the on-board codec and headphone output
- LCD status page with live audio runtime information

What the current firmware does not attempt yet:

- microphone/input USB Audio
- dynamic sample-rate switching beyond the fixed 48 kHz path
- advanced UI pages or user interaction

## 15. Files Most Likely To Be Modified During Secondary Development

### Audio features

- [usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
- [usbd_audio_if.h](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.h)
- [usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

### UI / Display

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- [st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
- [lv_port_disp_st7735s.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_port_disp_st7735s.c)
- [lv_conf.h](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/Third_Party/LVGL/lv_conf.h)

### RTOS / system tuning

- [main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
- [FreeRTOSConfig.h](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Inc/FreeRTOSConfig.h)

## 16. Suggested Next Improvements

Useful future work items:

- migrate LCD from GPIO bit-bang to hardware SPI
- clean up LVGL configuration warnings and version-specific options
- expose actual host-selected sample rate if variable sample-rate support is added
- add structured logging/debug LEDs for audio state transitions
- add a dedicated service layer for UI model updates
- document measured CPU load and DMA timing margins

## 17. Quick Start For New Contributors

If you are new to this project, start in this order:

1. Read [Core/Src/main.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/main.c)
2. Read [USB_DEVICE/App/usbd_audio_if.c](/opt/coding/stm32/STM32F407-DISC-Audio/USB_DEVICE/App/usbd_audio_if.c)
3. Read [Core/Src/st7735_lcd.c](/opt/coding/stm32/STM32F407-DISC-Audio/Core/Src/st7735_lcd.c)
4. Read [Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c](/opt/coding/stm32/STM32F407-DISC-Audio/Middlewares/ST/STM32_USB_Device_Library/Class/AUDIO/Src/usbd_audio.c)

This order gives the clearest mental model:

- task structure
- audio control flow
- display transport
- middleware behavior

