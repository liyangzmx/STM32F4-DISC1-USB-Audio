#include "oled_display.h"
#include "main.h"

#include <string.h>

#define OLED_I2C_ADDR         (0x3CU << 1)
#define OLED_CTRL_CMD         0x00U
#define OLED_CTRL_DATA_STREAM 0x40U

extern I2C_HandleTypeDef hi2c2;

static void oled_gpio_prepare(void);
static void oled_reset(void);
static HAL_StatusTypeDef oled_write_command(uint8_t cmd);
static HAL_StatusTypeDef oled_write_command_list(const uint8_t *cmds, uint16_t len);
static HAL_StatusTypeDef oled_write_data(const uint8_t *data, uint16_t len);
static void oled_set_page(uint8_t page);

static void oled_gpio_prepare(void)
{
  HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_SET);
}

static void oled_reset(void)
{
  HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(100);
}

static HAL_StatusTypeDef oled_write_command(uint8_t cmd)
{
  uint8_t packet[2];

  packet[0] = OLED_CTRL_CMD;
  packet[1] = cmd;

  return HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, packet, sizeof(packet), HAL_MAX_DELAY);
}

static HAL_StatusTypeDef oled_write_command_list(const uint8_t *cmds, uint16_t len)
{
  uint16_t i;

  for (i = 0; i < len; i++)
  {
    if (oled_write_command(cmds[i]) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

static HAL_StatusTypeDef oled_write_data(const uint8_t *data, uint16_t len)
{
  uint8_t tx_buf[1 + OLED_WIDTH];

  if (len > OLED_WIDTH)
  {
    return HAL_ERROR;
  }

  tx_buf[0] = OLED_CTRL_DATA_STREAM;
  memcpy(&tx_buf[1], data, len);

  return HAL_I2C_Master_Transmit(&hi2c2, OLED_I2C_ADDR, tx_buf, len + 1U, HAL_MAX_DELAY);
}

static void oled_set_page(uint8_t page)
{
  (void)oled_write_command((uint8_t)(0xB0U + page));
  (void)oled_write_command(0x00U);
  (void)oled_write_command(0x10U);
}

void OLED_Init(void)
{
  static const uint8_t init_cmds[] = {
      0xAEU,
      0xFDU, 0x12U,
      0xD5U, 0xA0U,
      0xA8U, 0x3FU,
      0xD3U, 0x00U,
      0x40U,
      0xA1U,
      0xC8U,
      0xDAU, 0x12U,
      0x81U, 0xBFU,
      0xD9U, 0x25U,
      0xDBU, 0x34U,
      0xA4U,
      0xA6U,
      0xAFU,
  };
  static const uint8_t clear_buf[OLED_WIDTH * OLED_PAGE_COUNT] = {0};

  oled_gpio_prepare();
  oled_reset();
  (void)oled_write_command_list(init_cmds, sizeof(init_cmds));
  OLED_UpdateFull(clear_buf);
}

void OLED_DisplayOn(void)
{
  (void)oled_write_command(0xAFU);
}

void OLED_DisplayOff(void)
{
  (void)oled_write_command(0xAEU);
}

void OLED_UpdateFull(const uint8_t *buffer)
{
  uint8_t page;

  for (page = 0U; page < OLED_PAGE_COUNT; page++)
  {
    oled_set_page(page);
    (void)oled_write_data(&buffer[(uint32_t)page * OLED_WIDTH], OLED_WIDTH);
  }
}
