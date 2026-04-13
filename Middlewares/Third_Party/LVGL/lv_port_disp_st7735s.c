/**
 * @file lv_port_disp_st7735s.c
 * LVGL display port for ST7735S LCD
 */

#include "lvgl.h"
#include "st7735_lcd.h"

/* Private function prototypes */
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**
 * Initialize the display port (LVGL v9 simplified API)
 */
void lv_port_disp_init(void)
{
    /* Initialize ST7735S hardware first */
    ST7735_Init();

    /* Create a display object using LVGL v9 API */
    lv_display_t *disp = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
    
    /* Set display color format (RGB565) */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    
    /* Set display rendering buffer */
    static uint8_t disp_buf[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10 * 2];
    lv_display_set_buffers(disp, disp_buf, NULL, sizeof(disp_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    /* Set display flush callback function */
    lv_display_set_flush_cb(disp, disp_flush);
}

/**
 * Flush the display buffer
 * Called by LVGL to update the display (LVGL v9 API)
 */
static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t x_start = area->x1;
    uint16_t y_start = area->y1;
    uint16_t x_end = area->x2;
    uint16_t y_end = area->y2;
    
    /* Draw the rectangle on the display */
    ST7735_FillRect(x_start, y_start, x_end, y_end, (const uint16_t *)px_map);

    /* Notify LVGL that flushing is complete */
    lv_display_flush_ready(disp);
}

