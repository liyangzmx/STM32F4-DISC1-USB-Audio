/**
 * @file lv_port_disp_oled.c
 * LVGL display port for 128x64 monochrome OLED
 */

#include "lvgl.h"
#include "draw/sw/lv_draw_sw_utils.h"
#include "oled_display.h"

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

void lv_port_disp_init(void)
{
    static uint8_t oled_draw_buf[(OLED_WIDTH * OLED_HEIGHT / 8U) + 8U];

    OLED_Init();

    lv_display_t *disp = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);
    lv_display_set_buffers(disp, oled_draw_buf, NULL, sizeof(oled_draw_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, disp_flush);
}

static void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    LV_UNUSED(area);

    static uint8_t oled_vtiled_buf[OLED_WIDTH * OLED_HEIGHT / 8U];

    px_map += 8;
    lv_draw_sw_i1_convert_to_vtiled(px_map,
                                    OLED_WIDTH * OLED_HEIGHT / 8U,
                                    OLED_WIDTH,
                                    OLED_HEIGHT,
                                    oled_vtiled_buf,
                                    sizeof(oled_vtiled_buf),
                                    true);
    OLED_UpdateFull(oled_vtiled_buf);
    lv_display_flush_ready(disp);
}
