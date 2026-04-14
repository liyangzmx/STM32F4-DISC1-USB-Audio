#ifndef __OLED_DISPLAY_H
#define __OLED_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OLED_WIDTH       128U
#define OLED_HEIGHT      64U
#define OLED_PAGE_COUNT  (OLED_HEIGHT / 8U)

void OLED_Init(void);
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);
void OLED_UpdateFull(const uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif
