#ifndef __ST7735_LCD_H
#define __ST7735_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LCD_COLOR_BLACK   0x0000U
#define LCD_COLOR_BLUE    0x001FU
#define LCD_COLOR_RED     0xF800U
#define LCD_COLOR_GREEN   0x07E0U
#define LCD_COLOR_CYAN    0x07FFU
#define LCD_COLOR_MAGENTA 0xF81FU
#define LCD_COLOR_YELLOW  0xFFE0U
#define LCD_COLOR_WHITE   0xFFFFU

void ST7735_Init(void);
void ST7735_FillScreen(uint16_t color);

#ifdef __cplusplus
}
#endif

#endif
