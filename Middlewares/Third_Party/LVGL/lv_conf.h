/**
 * @file lv_conf.h
 * LVGL configuration for STM32F407 with ST7735S display
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has a 8 bit interface (e.g. SPI)*/
#define LV_COLOR_16_SWAP 0

/* Enable more complex color processing */
#define LV_COLOR_MIX_ROUND_OFS 128

/*====================
   LED and KEY PINS
 *====================*/

/*====================
   DISPLAY SETTINGS
 *====================*/

/* Horizontal resolution of your display in pixels */
#define LV_HOR_RES_MAX         128

/* Vertical resolution of your display in pixels */
#define LV_VER_RES_MAX         160

/* Dot Per Inch: used to initialize default sizes. E.g. `lv_img_dsc_t` w and h . LV_DPI_DEF should be set to 100 then 1 dp = 1 cm
 * Set it to 0 if not known at design time. LV_DPI_DEF will be used then. */
#define LV_DPI_DEF            100

/* The type of coordinates. Should be `int16_t` (or `int32_t`) to avoid overflow when changing screen size */
#define LV_COORD_TYPE int16_t

/*====================
 * TEXT SETTINGS
 *====================*/

/* Enable UTF-8 encoded text support */
#define LV_USE_UNICODE 1

/* Handle unknown Unicode characters */
#define LV_UNICODE_REPLACEMENT_CHARACTER 0xFFFD

/*====================
 * MEMORY SETTINGS
 *====================*/

/* LittlevGL's internal memory manager's configurations */

/* 1: use custom malloc/free, 0: use the built-in `lv_mem_alloc` and `lv_mem_free` */
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    /* Size of the memory used by `lv_mem_alloc` in bytes (>= 2kB)*/
    #define LV_MEM_SIZE (48U * 1024U)

    /* Set an address for the memory pool instead of allocating it as a variable.
     * For example, you can use SRAM3 on Esp32-S3
     * #define LV_MEM_ADR 0x50000000 */
    #define LV_MEM_ADR 0x0
#else       /*LV_MEM_CUSTOM*/
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function to use*/
    #define LV_MEM_CUSTOM_ALLOC   malloc       /*Wrapper to malloc. Adjust it based on your implementation*/
    #define LV_MEM_CUSTOM_FREE    free         /*Wrapper to free. Adjust it based on your implementation*/
    #define LV_MEM_CUSTOM_REALLOC realloc      /*Wrapper to realloc. Adjust it based on your implementation*/
#endif     /*LV_MEM_CUSTOM*/

/*====================
 * HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds.
 * Can be changed in the display driver (`lv_disp_drv_init()`, `lv_disp_drv_update_init()`) */
#define LV_DISP_DEF_REFR_PERIOD 30      /*[ms]*/

/* Input device read period in milliseconds.
 * Can be changed in the display driver (`lv_indev_drv_init()`) */
#define LV_INDEV_DEF_READ_PERIOD 30     /*[ms]*/

/* Tick period in milliseconds indicating the rendering and gesture performers
 * has to be called periodically. The graphical objects and the animations
 * have to be updated in this period as well. For example every touch pad, button or API call
 * results an animation which internally has `anim_exec`. It takes `_anim_exec`to do not block
 * the rendering of the screen. So `LV_TICK_PERIOD` should be small enough to be <1/60000 that
 * way the animations will be done smoothly or ~5ms but not too small to neutralize filtering of application events.
 * One can adjust based on MCU performance. E.g. If it runs out of RAM increase the `LV_TICK_PERIOD` it will reduce the number of simultaneous animations. */
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /*Expression evaluating to current systime in ms*/
/* If LV_TICK_CUSTOM_SYS_TIME_EXPR is not defined, then `lv_tick_inc(lv_timer_handler())` should be called in a loop */

/* typedef a custom tick source type. It can be `uint32_t` or `uint16_t` etc */
#define LV_TICK_CUSTOM_SYS_TIME_TYPE uint32_t      /*Required if LV_TICK_CUSTOM_SYS_TIME_EXPR is set*/

/*====================
 * LOG SETTINGS
 *====================*/

/* Enable the log module */
#define LV_USE_LOG 1
#if LV_USE_LOG
    /* How important log should be added:
     * LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
     * LV_LOG_LEVEL_INFO        Log important events
     * LV_LOG_LEVEL_WARN        log if something unwanted happened but didn't cause crash
     * LV_LOG_LEVEL_ERROR       Only critical issue, when the system may crash
     * LV_LOG_LEVEL_USER        Only logs added by the user
     * LV_LOG_LEVEL_NONE        Do not log anything
     */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /* 1: Print the log with 'printf'; 0: user need to register a callback with `lv_log_register_print_cb()` */
    #define LV_LOG_PRINTF 0

    /* Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs */
    #define LV_LOG_TRACE_MEM            1
    #define LV_LOG_TRACE_TIMER          1
    #define LV_LOG_TRACE_INDEV          1
    #define LV_LOG_TRACE_DISP_REFR      1
    #define LV_LOG_TRACE_EVENT          1
    #define LV_LOG_TRACE_OBJ_CREATE     1
    #define LV_LOG_TRACE_LAYOUT         1
    #define LV_LOG_TRACE_ANIM           1

#endif  /*LV_USE_LOG*/

/*====================
 * ASSERTS
 *====================*/

/* Enable asserts if an operation in LVGL failed before. If enabled an `LV_ASSERT_HANDLER` will be called.
 * More commonly used by the LVGL developers. You can use it too in your application if you add `lv_assert.h`*/
#define LV_USE_ASSERT_NULL      1   /*Checks if the object is NULL*/
#define LV_USE_ASSERT_MALLOC    1   /*Checks if malloc failed*/
#define LV_USE_ASSERT_STYLE     1   /*Checks if a style is properly initialized*/
#define LV_USE_ASSERT_MEM_INTEGRITY 1   /*Check the integrity of `lv_mem` after critical operations*/
#define LV_USE_ASSERT_OBJ       0   /*Checks if the object's type/state is valid*/

/*====================
 * OTHERS
 *====================*/

/*Image decoder library*/
#define LV_USE_IMAGE 1
#define LV_USE_SPLIT_IMAGE 0

/* GIF image decoder. (requires `gifdec` library https://github.com/jckarter/gifdec) */
#define LV_USE_GIF 0

/* PNG image decoder. (requires `libpng` library) */
#define LV_USE_PNG 0

/* BMP image decoder. (requires `libbmp` library) */
#define LV_USE_BMP 0

/* SJPG image decoder. (requires `libjpeg` library) */
#define LV_USE_SJPG 0

/* WebP image decoder. (requires `libwebp` library) */
#define LV_USE_WEBP 0

/* QR code library. (requires `QR Code generator library https://github.com/nayuki/QR-Code-generator`) */
#define LV_USE_QR 0

#define LV_USE_FREETYPE 0

/*Rive animation library*/
#define LV_USE_RIVE 0

/*=====================
 * FONT USAGE
 *====================*/

/* Enable handling very large fonts (> 16 kB) */
#define LV_FONT_LARGE 0

/* Enable/Disable padding between the letter characters */
#define LV_FONT_FIT_TIGHT 0

/* Enable drawing placeholders when glyph dsc is not found */
#define LV_USE_FONT_PLACEHOLDER 1

#define LV_FONT_MONTSERRAT_8 1
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /**< bpp = 3*/

/*Pixel perfect monospace font*/
#define LV_FONT_UNSCII_8 0
#define LV_FONT_UNSCII_16 0

/* Demonstrate special features */
#define LV_FONT_UNSCII_8_COMPRESSED 0   /**< bpp = 4*/

#define LV_FONT_DEFAULT &lv_font_montserrat_12   /**< lv_conf.h can be changed at runtime. Then default font is used if not set by user*/

/*===================
 * TEXT SETTINGS
 *====================*/

/**
 * Select a character encoding for strings.
 * Your IDE or editor should have the same character encoding
 * - LV_TXT_ENC_UTF8
 * - LV_TXT_ENC_ASCII
 * - LV_TXT_ENC_UCS4
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*Can break (wrap) texts on these chars*/
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}!\'*+/:<=>%\\|`~"

/* If a word is at least this long, will break wherever "Word break" character is found */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/* Minimum number of characters in a long word to put on its own line */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/* Minimum number of characters from a long word to put on the next line */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* The control character to use for signalling text recoloring. */
#define LV_TXT_COLOR_CMD "#"

/*===================
 * Widget usage
 *====================*/
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_CANVAS 1
#define LV_USE_GAUGE 0
#define LV_USE_LINEMETER 0
#define LV_USE_SCALE 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_KEYBOARD 0
#define LV_USE_OBJX_NOMINMAX 0

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/
