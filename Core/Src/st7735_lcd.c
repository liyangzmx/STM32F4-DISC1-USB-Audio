#include "st7735_lcd.h"
#include "main.h"

#define ST7735_WIDTH     128U
#define ST7735_HEIGHT    160U
#define ST7735_X_OFFSET  2U
#define ST7735_Y_OFFSET  1U

/* 直接寄存器操作GPIO，比HAL更快 */
static inline void lcd_pin_high(GPIO_TypeDef *port, uint16_t pin)
{
  port->BSRR = pin;
}

static inline void lcd_pin_low(GPIO_TypeDef *port, uint16_t pin)
{
  port->BSRR = (uint32_t)pin << 16;
}

static inline void lcd_bus_delay(void)
{
  /* GPIO信号时序延迟 */
  uint8_t i;
  for (i = 0; i < 15; i++)
  {
    __NOP();
  }
}

static void lcd_gpio_init(void);
static void lcd_write_u8(uint8_t value);
static void lcd_write_command(uint8_t value);
static void lcd_write_data(uint8_t value);
static void lcd_reset(void);
static void lcd_init_sequence_st7735s(void);

static void lcd_gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  GPIO_InitStruct.Pin = LCD_CS_Pin | LCD_SCL_Pin | LCD_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_RST_Pin | LCD_DC_Pin | LCD_BLK_Pin;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  lcd_pin_high(LCD_CS_GPIO_Port, LCD_CS_Pin);
  lcd_pin_high(LCD_SCL_GPIO_Port, LCD_SCL_Pin);
  lcd_pin_high(LCD_SDA_GPIO_Port, LCD_SDA_Pin);
  lcd_pin_high(LCD_DC_GPIO_Port, LCD_DC_Pin);
  lcd_pin_high(LCD_RST_GPIO_Port, LCD_RST_Pin);
  lcd_pin_high(LCD_BLK_GPIO_Port, LCD_BLK_Pin);
}

static void lcd_write_u8(uint8_t value)
{
  uint32_t bit;

  for (bit = 0; bit < 8U; ++bit)
  {
    if ((value & 0x80U) != 0U)
    {
      lcd_pin_high(LCD_SDA_GPIO_Port, LCD_SDA_Pin);
    }
    else
    {
      lcd_pin_low(LCD_SDA_GPIO_Port, LCD_SDA_Pin);
    }

    lcd_pin_low(LCD_SCL_GPIO_Port, LCD_SCL_Pin);
    lcd_bus_delay();
    lcd_pin_high(LCD_SCL_GPIO_Port, LCD_SCL_Pin);
    lcd_bus_delay();
    value <<= 1;
  }
}

static void lcd_write_command(uint8_t value)
{
  lcd_pin_low(LCD_CS_GPIO_Port, LCD_CS_Pin);
  lcd_pin_low(LCD_DC_GPIO_Port, LCD_DC_Pin);
  lcd_write_u8(value);
  lcd_pin_high(LCD_CS_GPIO_Port, LCD_CS_Pin);
}

static void lcd_write_data(uint8_t value)
{
  lcd_pin_low(LCD_CS_GPIO_Port, LCD_CS_Pin);
  lcd_pin_high(LCD_DC_GPIO_Port, LCD_DC_Pin);
  lcd_write_u8(value);
  lcd_pin_high(LCD_CS_GPIO_Port, LCD_CS_Pin);
}

static void lcd_reset(void)
{
  lcd_pin_high(LCD_RST_GPIO_Port, LCD_RST_Pin);
  HAL_Delay(1);
  lcd_pin_low(LCD_RST_GPIO_Port, LCD_RST_Pin);
  HAL_Delay(1);
  lcd_pin_high(LCD_RST_GPIO_Port, LCD_RST_Pin);
  HAL_Delay(120);
}

static void lcd_init_sequence_st7735s(void)
{
  lcd_write_command(0x11);  // Sleep Out
  HAL_Delay(120);

  lcd_write_command(0xB1);  // Frame Rate Control (normal mode)
  lcd_write_data(0x05);
  lcd_write_data(0x3C);
  lcd_write_data(0x3C);

  lcd_write_command(0xB2);  // Frame Rate Control (idle mode)
  lcd_write_data(0x05);
  lcd_write_data(0x3C);
  lcd_write_data(0x3C);

  lcd_write_command(0xB3);  // Frame Rate Control (partial mode)
  lcd_write_data(0x05);
  lcd_write_data(0x3C);
  lcd_write_data(0x3C);
  lcd_write_data(0x05);
  lcd_write_data(0x3C);
  lcd_write_data(0x3C);

  lcd_write_command(0xB4);  // Inversion Control
  lcd_write_data(0x03);

  lcd_write_command(0xC0);  // Power Control 1
  lcd_write_data(0x28);
  lcd_write_data(0x08);
  lcd_write_data(0x04);

  lcd_write_command(0xC1);  // Power Control 2
  lcd_write_data(0xC0);

  lcd_write_command(0xC2);  // Power Control 3
  lcd_write_data(0x0D);
  lcd_write_data(0x00);

  lcd_write_command(0xC3);  // Power Control 4
  lcd_write_data(0x8D);
  lcd_write_data(0x2A);

  lcd_write_command(0xC4);  // Power Control 5
  lcd_write_data(0x8D);
  lcd_write_data(0xEE);

  lcd_write_command(0xC5);  // VCOM Control
  lcd_write_data(0x1A);

  lcd_write_command(0x36);  // Memory Access Control
  lcd_write_data(0xC0);

  lcd_write_command(0xE0);  // Gamma Correction (positive)
  lcd_write_data(0x04);
  lcd_write_data(0x22);
  lcd_write_data(0x07);
  lcd_write_data(0x0A);
  lcd_write_data(0x2E);
  lcd_write_data(0x30);
  lcd_write_data(0x25);
  lcd_write_data(0x2A);
  lcd_write_data(0x28);
  lcd_write_data(0x26);
  lcd_write_data(0x2E);
  lcd_write_data(0x3A);
  lcd_write_data(0x00);
  lcd_write_data(0x01);
  lcd_write_data(0x03);
  lcd_write_data(0x13);

  lcd_write_command(0xE1);  // Gamma Correct (negative)
  lcd_write_data(0x04);
  lcd_write_data(0x16);
  lcd_write_data(0x06);
  lcd_write_data(0x0D);
  lcd_write_data(0x2D);
  lcd_write_data(0x26);
  lcd_write_data(0x23);
  lcd_write_data(0x27);
  lcd_write_data(0x27);
  lcd_write_data(0x25);
  lcd_write_data(0x2D);
  lcd_write_data(0x3B);
  lcd_write_data(0x00);
  lcd_write_data(0x01);
  lcd_write_data(0x04);
  lcd_write_data(0x13);

  lcd_write_command(0x3A);  // Interface Pixel Format
  lcd_write_data(0x05);

  lcd_write_command(0x29);  // Display ON
}

void ST7735_Init(void)
{
  lcd_gpio_init();
  lcd_reset();
  lcd_init_sequence_st7735s();
  HAL_Delay(20);
}

void ST7735_FillScreen(uint16_t color)
{
  uint32_t i;

  // 设置窗口为整个屏幕
  lcd_write_command(0x2A);  // Column Address Set
  lcd_write_data(0x00);
  lcd_write_data(0x00 + ST7735_X_OFFSET);
  lcd_write_data(0x00);
  lcd_write_data((uint8_t)(ST7735_WIDTH - 1 + ST7735_X_OFFSET));

  lcd_write_command(0x2B);  // Row Address Set
  lcd_write_data(0x00);
  lcd_write_data(0x00 + ST7735_Y_OFFSET);
  lcd_write_data(0x00);
  lcd_write_data((uint8_t)(ST7735_HEIGHT - 1 + ST7735_Y_OFFSET));

  // 开始写入GRAM，保持CS低电平
  lcd_pin_low(LCD_CS_GPIO_Port, LCD_CS_Pin);
  lcd_pin_low(LCD_DC_GPIO_Port, LCD_DC_Pin);
  lcd_bus_delay();
  
  lcd_write_u8(0x2C);  // Memory Write command
  
  lcd_bus_delay();
  lcd_pin_high(LCD_DC_GPIO_Port, LCD_DC_Pin);  // Switch to data mode
  lcd_bus_delay();

  // 填充所有像素（CS保持低）
  for (i = 0; i < (uint32_t)ST7735_WIDTH * (uint32_t)ST7735_HEIGHT; i++)
  {
    lcd_write_u8((uint8_t)(color >> 8));
    lcd_write_u8((uint8_t)color);
  }

  lcd_bus_delay();
  lcd_pin_high(LCD_CS_GPIO_Port, LCD_CS_Pin);  // Release CS
}
