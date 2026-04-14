/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_audio_if.c
  * @version        : v1.0_Cube
  * @brief          : Generic media access layer.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio_if.h"

/* USER CODE BEGIN INCLUDE */
#include "main.h"
#include <string.h>

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_AUDIO_IF
  * @{
  */

/** @defgroup USBD_AUDIO_IF_Private_TypesDefinitions USBD_AUDIO_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_Defines USBD_AUDIO_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
#define CS43L22_I2C_ADDR              (0x4AU << 1)
#define CS43L22_OUTPUT_HEADPHONE      0xAFU
#define CS43L22_I2C_TIMEOUT_MS        100U
#define AUDIO_USB_PACKET_TIMEOUT_MS   20U
#define AUDIO_CODEC_FIXED_VOLUME      82U
#define AUDIO_CODEC_PCM_VOLUME        0x00U

#define CS43L22_REG_POWER_CTL1        0x02U
#define CS43L22_REG_POWER_CTL2        0x04U
#define CS43L22_REG_CLOCKING_CTL      0x05U
#define CS43L22_REG_INTERFACE_CTL1    0x06U
#define CS43L22_REG_ANALOG_ZC_SR_SETT 0x0AU
#define CS43L22_REG_PLAYBACK_CTL1     0x0DU
#define CS43L22_REG_MISC_CTL          0x0EU
#define CS43L22_REG_PLAYBACK_CTL2     0x0FU
#define CS43L22_REG_LIMIT_CTL1        0x27U
#define CS43L22_REG_MISC_MAGIC_32     0x32U
#define CS43L22_REG_MISC_MAGIC_47     0x47U
#define CS43L22_REG_TONE_CTL          0x1FU
#define CS43L22_REG_MASTER_A_VOL      0x20U
#define CS43L22_REG_MASTER_B_VOL      0x21U
#define CS43L22_REG_HEADPHONE_A_VOL   0x22U
#define CS43L22_REG_HEADPHONE_B_VOL   0x23U
#define CS43L22_REG_PCMA_VOL          0x1AU
#define CS43L22_REG_PCMB_VOL          0x1BU

#define AUDIO_UI_STATE_IDLE           0U
#define AUDIO_UI_STATE_STARTING       1U
#define AUDIO_UI_STATE_ACTIVE         2U

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_Macros USBD_AUDIO_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_Variables USBD_AUDIO_IF_Private_Variables
  * @brief Private variables.
  * @{
  */

/* USER CODE BEGIN PRIVATE_VARIABLES */
static uint8_t audio_muted = 0U;
static uint8_t codec_initialized = 0U;
static uint8_t codec_playing = 0U;
static uint8_t audio_volume = AUDIO_CODEC_FIXED_VOLUME;
static uint8_t playback_start_pending = 0U;
static volatile uint8_t codec_init_requested = 0U;
static volatile uint8_t playback_dma_start_requested = 0U;
static volatile uint8_t playback_stop_requested = 0U;
static volatile uint8_t codec_deinit_requested = 0U;
static volatile uint32_t last_usb_packet_tick = 0U;
static volatile uint32_t audio_ui_update_counter = 0U;
static volatile uint8_t audio_ui_playback_state = AUDIO_UI_STATE_IDLE;

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Exported_Variables USBD_AUDIO_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_IF_Private_FunctionPrototypes USBD_AUDIO_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t AUDIO_Init_FS(uint32_t AudioFreq, uint32_t Volume, uint32_t options);
static int8_t AUDIO_DeInit_FS(uint32_t options);
static int8_t AUDIO_AudioCmd_FS(uint8_t* pbuf, uint32_t size, uint8_t cmd);
static int8_t AUDIO_VolumeCtl_FS(uint8_t vol);
static int8_t AUDIO_MuteCtl_FS(uint8_t cmd);
static int8_t AUDIO_PeriodicTC_FS(uint8_t *pbuf, uint32_t size, uint8_t cmd);
static int8_t AUDIO_GetState_FS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
static int8_t CS43L22_Init(uint8_t volume);
static void CS43L22_DeInit(void);
static int8_t CS43L22_Play(void);
static int8_t CS43L22_Stop(void);
static int8_t CS43L22_SetVolume(uint8_t volume);
static int8_t CS43L22_SetMute(uint8_t muted);
static int8_t CS43L22_Write(uint8_t reg, uint8_t value);
static int8_t CS43L22_Read(uint8_t reg, uint8_t *value);
static uint8_t CS43L22_ConvertVolume(uint8_t volume);
static void AUDIO_ClearUsbRingBuffer(void);
static int8_t AUDIO_StartPlaybackDma(void);
static void AUDIO_StopPlaybackPath(void);
static void AUDIO_UI_NotifyChanged(void);

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops_FS =
{
  AUDIO_Init_FS,
  AUDIO_DeInit_FS,
  AUDIO_AudioCmd_FS,
  AUDIO_VolumeCtl_FS,
  AUDIO_MuteCtl_FS,
  AUDIO_PeriodicTC_FS,
  AUDIO_GetState_FS,
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the AUDIO media low layer over USB FS IP
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param  options: Reserved for future use
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_Init_FS(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
  /* USER CODE BEGIN 0 */
  UNUSED(options);

  if (AudioFreq != USBD_AUDIO_FREQ)
  {
    return (USBD_FAIL);
  }

  audio_volume = (uint8_t)Volume;
  audio_volume = AUDIO_CODEC_FIXED_VOLUME;
  audio_muted = 0U;
  codec_playing = 0U;
  playback_start_pending = 0U;
  codec_init_requested = 0U;
  playback_dma_start_requested = 0U;
  playback_stop_requested = 0U;
  codec_deinit_requested = 0U;
  last_usb_packet_tick = HAL_GetTick();
  audio_ui_playback_state = AUDIO_UI_STATE_IDLE;
  AUDIO_ClearUsbRingBuffer();
  AUDIO_UI_NotifyChanged();

  return (USBD_OK);
  /* USER CODE END 0 */
}

/**
  * @brief  De-Initializes the AUDIO media low layer
  * @param  options: Reserved for future use
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_DeInit_FS(uint32_t options)
{
  /* USER CODE BEGIN 1 */
  UNUSED(options);
  playback_stop_requested = 1U;
  codec_deinit_requested = 1U;
  audio_ui_playback_state = AUDIO_UI_STATE_IDLE;
  AUDIO_UI_NotifyChanged();
  return (USBD_OK);
  /* USER CODE END 1 */
}

/**
  * @brief  Handles AUDIO command.
  * @param  pbuf: Pointer to buffer of data to be sent
  * @param  size: Number of data to be sent (in bytes)
  * @param  cmd: Command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_AudioCmd_FS(uint8_t* pbuf, uint32_t size, uint8_t cmd)
{
  /* USER CODE BEGIN 2 */
  HAL_StatusTypeDef status = HAL_OK;

  switch(cmd)
  {
    case AUDIO_CMD_START:
      UNUSED(pbuf);
      UNUSED(size);
      playback_stop_requested = 0U;
      codec_deinit_requested = 0U;
      playback_dma_start_requested = 0U;
      last_usb_packet_tick = HAL_GetTick();
      codec_init_requested = 1U;
      audio_ui_playback_state = AUDIO_UI_STATE_STARTING;
      AUDIO_UI_NotifyChanged();
    break;

    case AUDIO_CMD_PLAY:
      /*
       * DMA is circular over the USB audio ring buffer; the USB stack keeps
       * filling the inactive half while I2S drains the active half.
       */
      UNUSED(pbuf);
      UNUSED(size);
    break;

    case AUDIO_CMD_STOP:
      playback_dma_start_requested = 0U;
      playback_stop_requested = 1U;
      audio_ui_playback_state = AUDIO_UI_STATE_IDLE;
      AUDIO_UI_NotifyChanged();
      status = HAL_OK;
    break;

    default:
      status = HAL_ERROR;
    break;
  }

  return (status == HAL_OK) ? (USBD_OK) : (USBD_FAIL);
  /* USER CODE END 2 */
}

/**
  * @brief  Controls AUDIO Volume.
  * @param  vol: volume level (0..100)
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_VolumeCtl_FS(uint8_t vol)
{
  /* USER CODE BEGIN 3 */
  audio_volume = vol;
  AUDIO_UI_NotifyChanged();

  if (codec_initialized == 0U)
  {
    return (USBD_OK);
  }

  return CS43L22_SetVolume(vol);
  /* USER CODE END 3 */
}

/**
  * @brief  Controls AUDIO Mute.
  * @param  cmd: command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_MuteCtl_FS(uint8_t cmd)
{
  /* USER CODE BEGIN 4 */
  audio_muted = (cmd != 0U) ? 1U : 0U;
  AUDIO_UI_NotifyChanged();

  if (codec_initialized == 0U)
  {
    return (USBD_OK);
  }

  return CS43L22_SetMute(cmd);
  /* USER CODE END 4 */
}

/**
  * @brief  AUDIO_PeriodicT_FS
  * @param  cmd: Command opcode
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_PeriodicTC_FS(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
  /* USER CODE BEGIN 5 */
  USBD_AUDIO_HandleTypeDef *haudio;
  uint32_t next_wr_ptr;

  UNUSED(pbuf);

  if (cmd == AUDIO_OUT_TC)
  {
    last_usb_packet_tick = HAL_GetTick();
  }

  if ((cmd != AUDIO_OUT_TC) || (playback_start_pending == 0U) || (codec_playing != 0U))
  {
    return (USBD_OK);
  }

  haudio = (USBD_AUDIO_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

  if (haudio == NULL)
  {
    return (USBD_FAIL);
  }

  next_wr_ptr = (uint32_t)haudio->wr_ptr + size;
  if (next_wr_ptr >= AUDIO_TOTAL_BUF_SIZE)
  {
    next_wr_ptr -= AUDIO_TOTAL_BUF_SIZE;
  }

  if (next_wr_ptr < (AUDIO_TOTAL_BUF_SIZE / 2U))
  {
    return (USBD_OK);
  }

  playback_dma_start_requested = 1U;
  return (USBD_OK);

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Gets AUDIO State.
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t AUDIO_GetState_FS(void)
{
  /* USER CODE BEGIN 6 */
  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  Manages the DMA full transfer complete event.
  * @retval None
  */
void TransferComplete_CallBack_FS(void)
{
  /* USER CODE BEGIN 7 */
  if (codec_playing == 0U)
  {
    return;
  }

  USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_FULL);
  /* USER CODE END 7 */
}

/**
  * @brief  Manages the DMA Half transfer complete event.
  * @retval None
  */
void HalfTransfer_CallBack_FS(void)
{
  /* USER CODE BEGIN 8 */
  if (codec_playing == 0U)
  {
    return;
  }

  USBD_AUDIO_Sync(&hUsbDeviceFS, AUDIO_OFFSET_HALF);
  /* USER CODE END 8 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
void AUDIO_ServiceTaskStep(void)
{
  if (((codec_playing != 0U) || (playback_start_pending != 0U) ||
       (codec_init_requested != 0U) || (playback_dma_start_requested != 0U)) &&
      ((HAL_GetTick() - last_usb_packet_tick) > AUDIO_USB_PACKET_TIMEOUT_MS))
  {
    playback_stop_requested = 1U;
    codec_deinit_requested = 1U;
  }

  if (playback_stop_requested != 0U)
  {
    AUDIO_StopPlaybackPath();

    if (codec_deinit_requested != 0U)
    {
      CS43L22_DeInit();
    }

    codec_init_requested = 0U;
    playback_dma_start_requested = 0U;
    playback_stop_requested = 0U;
    codec_deinit_requested = 0U;
    return;
  }

  if (codec_init_requested != 0U)
  {
    if ((codec_initialized != 0U) || (CS43L22_Init(audio_volume) == USBD_OK))
    {
      codec_init_requested = 0U;
      playback_start_pending = 1U;
    }

    return;
  }

  if ((playback_dma_start_requested != 0U) && (codec_playing == 0U))
  {
    if (AUDIO_StartPlaybackDma() == USBD_OK)
    {
      playback_dma_start_requested = 0U;
    }
  }
}

uint8_t AUDIO_UI_GetVolume(void)
{
  return audio_volume;
}

uint8_t AUDIO_UI_GetMuted(void)
{
  return audio_muted;
}

uint8_t AUDIO_UI_GetPlaying(void)
{
  return codec_playing;
}

uint8_t AUDIO_UI_GetCodecReady(void)
{
  return codec_initialized;
}

uint32_t AUDIO_UI_GetSampleRate(void)
{
  return USBD_AUDIO_FREQ;
}

uint8_t AUDIO_UI_GetPlaybackState(void)
{
  return audio_ui_playback_state;
}

uint32_t AUDIO_UI_GetUpdateCounter(void)
{
  return audio_ui_update_counter;
}

static void AUDIO_UI_NotifyChanged(void)
{
  audio_ui_update_counter++;
  UI_RequestAudioStatusRefresh();
}

static void AUDIO_ClearUsbRingBuffer(void)
{
  USBD_AUDIO_HandleTypeDef *haudio;

  haudio = (USBD_AUDIO_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

  if (haudio == NULL)
  {
    return;
  }

  memset(haudio->buffer, 0, sizeof(haudio->buffer));
}

static int8_t AUDIO_StartPlaybackDma(void)
{
  HAL_StatusTypeDef status;
  USBD_AUDIO_HandleTypeDef *haudio;

  haudio = (USBD_AUDIO_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

  if (haudio == NULL)
  {
    return USBD_FAIL;
  }

  if (CS43L22_Play() != USBD_OK)
  {
    return USBD_FAIL;
  }

  status = HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)haudio->buffer,
                                AUDIO_TOTAL_BUF_SIZE / sizeof(uint16_t));
  codec_playing = (status == HAL_OK) ? 1U : 0U;

  if (codec_playing != 0U)
  {
    playback_start_pending = 0U;
    audio_ui_playback_state = AUDIO_UI_STATE_ACTIVE;
    AUDIO_UI_NotifyChanged();
    return USBD_OK;
  }

  return USBD_FAIL;
}

static void AUDIO_StopPlaybackPath(void)
{
  codec_playing = 0U;
  playback_start_pending = 0U;
  playback_dma_start_requested = 0U;
  audio_ui_playback_state = AUDIO_UI_STATE_IDLE;
  AUDIO_UI_NotifyChanged();

  /*
   * When USB disappears, SPI/I2S DMA may still loop the last audio fragment
   * for a short moment before the stop sequence fully takes effect. Clear the
   * shared ring buffer first so any tail immediately becomes silence.
   */
  AUDIO_ClearUsbRingBuffer();

  if (codec_initialized != 0U)
  {
    (void)CS43L22_Write(CS43L22_REG_POWER_CTL2, 0xFFU);
    (void)CS43L22_Write(CS43L22_REG_HEADPHONE_A_VOL, 0x01U);
    (void)CS43L22_Write(CS43L22_REG_HEADPHONE_B_VOL, 0x01U);
    HAL_Delay(2U);
  }

  (void)HAL_I2S_DMAStop(&hi2s3);
  (void)CS43L22_Stop();
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance == SPI3)
  {
    HalfTransfer_CallBack_FS();
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance == SPI3)
  {
    TransferComplete_CallBack_FS();
  }
}

static int8_t CS43L22_Init(uint8_t volume)
{
  uint8_t reg_value = 0U;
  UNUSED(volume);

  HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(5U);
  HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(5U);

  if (CS43L22_Write(CS43L22_REG_POWER_CTL1, 0x01U) != USBD_OK)
  {
    return (USBD_FAIL);
  }

  if ((CS43L22_Write(0x00U, 0x99U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_MISC_MAGIC_47, 0x80U) != USBD_OK) ||
      (CS43L22_Read(CS43L22_REG_MISC_MAGIC_32, &reg_value) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_MISC_MAGIC_32, (uint8_t)(reg_value | 0x80U)) != USBD_OK) ||
      (CS43L22_Read(CS43L22_REG_MISC_MAGIC_32, &reg_value) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_MISC_MAGIC_32, (uint8_t)(reg_value & (uint8_t)~0x80U)) != USBD_OK) ||
      (CS43L22_Write(0x00U, 0x00U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_POWER_CTL2, 0xFFU) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_CLOCKING_CTL, 0x81U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_INTERFACE_CTL1, 0x07U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_ANALOG_ZC_SR_SETT, 0x00U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_PLAYBACK_CTL1, 0x00U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_MISC_CTL, 0x04U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_PLAYBACK_CTL2, 0x00U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_LIMIT_CTL1, 0x00U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_TONE_CTL, 0x0FU) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_PCMA_VOL, AUDIO_CODEC_PCM_VOLUME) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_PCMB_VOL, AUDIO_CODEC_PCM_VOLUME) != USBD_OK) ||
      (CS43L22_SetVolume(AUDIO_CODEC_FIXED_VOLUME) != USBD_OK) ||
      (CS43L22_SetMute(audio_muted) != USBD_OK))
  {
    return (USBD_FAIL);
  }

  codec_initialized = 1U;
  AUDIO_UI_NotifyChanged();
  return (USBD_OK);
}

static void CS43L22_DeInit(void)
{
  codec_initialized = 0U;
  HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_RESET);
  AUDIO_UI_NotifyChanged();
}

static int8_t CS43L22_Play(void)
{
  if (codec_initialized == 0U)
  {
    return (USBD_FAIL);
  }

  if ((CS43L22_Write(CS43L22_REG_MISC_CTL, 0x06U) != USBD_OK) ||
      (CS43L22_SetMute(audio_muted) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_POWER_CTL1, 0x9EU) != USBD_OK))
  {
    return (USBD_FAIL);
  }

  return (USBD_OK);
}

static int8_t CS43L22_Stop(void)
{
  if (codec_initialized == 0U)
  {
    return (USBD_OK);
  }

  if ((CS43L22_SetMute(1U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_MISC_CTL, 0x04U) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_POWER_CTL1, 0x9FU) != USBD_OK))
  {
    return (USBD_FAIL);
  }

  return (USBD_OK);
}

static int8_t CS43L22_SetVolume(uint8_t volume)
{
  uint8_t converted_volume = CS43L22_ConvertVolume(volume);
  audio_volume = volume;

  if (converted_volume > 0xE6U)
  {
    converted_volume = (uint8_t)(converted_volume - 0xE7U);
  }
  else
  {
    converted_volume = (uint8_t)(converted_volume + 0x19U);
  }

  if ((CS43L22_Write(CS43L22_REG_MASTER_A_VOL, converted_volume) != USBD_OK) ||
      (CS43L22_Write(CS43L22_REG_MASTER_B_VOL, converted_volume) != USBD_OK))
  {
    return (USBD_FAIL);
  }

  return (USBD_OK);
}

static int8_t CS43L22_SetMute(uint8_t muted)
{
  audio_muted = (muted != 0U) ? 1U : 0U;

  if (audio_muted != 0U)
  {
    if ((CS43L22_Write(CS43L22_REG_POWER_CTL2, 0xFFU) != USBD_OK) ||
        (CS43L22_Write(CS43L22_REG_HEADPHONE_A_VOL, 0x01U) != USBD_OK) ||
        (CS43L22_Write(CS43L22_REG_HEADPHONE_B_VOL, 0x01U) != USBD_OK))
    {
      return (USBD_FAIL);
    }
  }
  else
  {
    if ((CS43L22_Write(CS43L22_REG_HEADPHONE_A_VOL, 0x00U) != USBD_OK) ||
        (CS43L22_Write(CS43L22_REG_HEADPHONE_B_VOL, 0x00U) != USBD_OK) ||
        (CS43L22_Write(CS43L22_REG_POWER_CTL2, CS43L22_OUTPUT_HEADPHONE) != USBD_OK))
    {
      return (USBD_FAIL);
    }
  }

  return (USBD_OK);
}

static int8_t CS43L22_Write(uint8_t reg, uint8_t value)
{
  if (HAL_I2C_Mem_Write(&hi2c1, CS43L22_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                        &value, 1U, CS43L22_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return (USBD_FAIL);
  }

  return (USBD_OK);
}

static int8_t CS43L22_Read(uint8_t reg, uint8_t *value)
{
  if (HAL_I2C_Mem_Read(&hi2c1, CS43L22_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                       value, 1U, CS43L22_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return (USBD_FAIL);
  }

  return (USBD_OK);
}

static uint8_t CS43L22_ConvertVolume(uint8_t volume)
{
  if (volume > 100U)
  {
    return 255U;
  }

  return (uint8_t)(((uint32_t)volume * 255U) / 100U);
}

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */
