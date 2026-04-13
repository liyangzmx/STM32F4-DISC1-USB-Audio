/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include "st7735_lcd.h"
#include "usb_device.h"
#include "usbd_audio_if.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lvgl.h"
#include "lv_port_disp_st7735s.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

osThreadId defaultTaskHandle;
osThreadId audioTaskHandle;
osThreadId lcdTaskHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
void StartDefaultTask(void const * argument);
void StartAudioTask(void const * argument);
void StartLcdTask(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S3_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of audioTask */
  osThreadDef(audioTask, StartAudioTask, osPriorityAboveNormal, 0, 128);
  audioTaskHandle = osThreadCreate(osThread(audioTask), NULL);

  /* definition and creation of lcdTask */
  osThreadDef(lcdTask, StartLcdTask, osPriorityLow, 0, 256);
  lcdTaskHandle = osThreadCreate(osThread(lcdTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_MISO_Pin SPI1_MOSI_Pin */
  GPIO_InitStruct.Pin = SPI1_MISO_Pin|SPI1_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief Create USB Audio Information UI
 */
static void create_audio_info_ui(void)
{
  /* Get active screen */
  lv_obj_t *scr = lv_scr_act();
  
  /* Set background color to black */
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  
  /* TEST: Add simple colored squares to verify rendering */
  /* Red square - top left */
  lv_obj_t *red_box = lv_obj_create(scr);
  lv_obj_set_size(red_box, 30, 30);
  lv_obj_set_align(red_box, LV_ALIGN_TOP_LEFT);
  lv_obj_set_pos(red_box, 0, 0);
  lv_obj_set_style_bg_color(red_box, lv_color_hex(0xFF0000), LV_PART_MAIN);
  lv_obj_set_style_border_width(red_box, 0, LV_PART_MAIN);
  
  /* Green square - top center */
  lv_obj_t *green_box = lv_obj_create(scr);
  lv_obj_set_size(green_box, 30, 30);
  lv_obj_set_align(green_box, LV_ALIGN_TOP_MID);
  lv_obj_set_pos(green_box, 0, 10);
  lv_obj_set_style_bg_color(green_box, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_set_style_border_width(green_box, 0, LV_PART_MAIN);
  
  /* Blue square - top right */
  lv_obj_t *blue_box = lv_obj_create(scr);
  lv_obj_set_size(blue_box, 30, 30);
  lv_obj_set_align(blue_box, LV_ALIGN_TOP_RIGHT);
  lv_obj_set_pos(blue_box, 0, 0);
  lv_obj_set_style_bg_color(blue_box, lv_color_hex(0x0000FF), LV_PART_MAIN);
  lv_obj_set_style_border_width(blue_box, 0, LV_PART_MAIN);
  
  /* Yellow square - center */
  lv_obj_t *yellow_box = lv_obj_create(scr);
  lv_obj_set_size(yellow_box, 40, 40);
  lv_obj_align(yellow_box, LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_style_bg_color(yellow_box, lv_color_hex(0xFFFF00), LV_PART_MAIN);
  lv_obj_set_style_border_width(yellow_box, 0, LV_PART_MAIN);
  
  /* Cyan square - bottom left */
  lv_obj_t *cyan_box = lv_obj_create(scr);
  lv_obj_set_size(cyan_box, 30, 30);
  lv_obj_align(cyan_box, LV_ALIGN_BOTTOM_LEFT, 5, -5);
  lv_obj_set_style_bg_color(cyan_box, lv_color_hex(0x00FFFF), LV_PART_MAIN);
  lv_obj_set_style_border_width(cyan_box, 0, LV_PART_MAIN);
  
  /* Magenta square - bottom right */
  lv_obj_t *magenta_box = lv_obj_create(scr);
  lv_obj_set_size(magenta_box, 30, 30);
  lv_obj_align(magenta_box, LV_ALIGN_BOTTOM_RIGHT, -5, -5);
  lv_obj_set_style_bg_color(magenta_box, lv_color_hex(0xFF00FF), LV_PART_MAIN);
  lv_obj_set_style_border_width(magenta_box, 0, LV_PART_MAIN);
  
  /* Simple test label in the middle */
  lv_obj_t *test_label = lv_label_create(scr);
  lv_label_set_text(test_label, "LVGL OK");
  lv_obj_align(test_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_style_text_color(test_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(test_label, &lv_font_montserrat_14, LV_PART_MAIN);
  
  /* Title Label: "USB Audio Info" */
  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "USB AUDIO");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 45);
  lv_obj_set_style_text_color(title, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
  
  /* Horizontal separator line */
  lv_obj_t *line1 = lv_obj_create(scr);
  lv_obj_set_size(line1, 128, 2);
  lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 62);
  lv_obj_set_style_bg_color(line1, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_set_style_border_width(line1, 0, LV_PART_MAIN);
  
  /* USB Status Label */
  lv_obj_t *usb_status_label = lv_label_create(scr);
  lv_label_set_text(usb_status_label, "USB:");
  lv_obj_align(usb_status_label, LV_ALIGN_TOP_LEFT, 5, 70);
  lv_obj_set_style_text_color(usb_status_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_style_text_font(usb_status_label, &lv_font_montserrat_12, LV_PART_MAIN);
  
  /* USB Status Value */
  lv_obj_t *usb_status_val = lv_label_create(scr);
  lv_label_set_text(usb_status_val, "OK");
  lv_obj_align(usb_status_val, LV_ALIGN_TOP_RIGHT, -5, 70);
  lv_obj_set_style_text_color(usb_status_val, lv_color_hex(0xFFFF00), LV_PART_MAIN);
  lv_obj_set_style_text_font(usb_status_val, &lv_font_montserrat_12, LV_PART_MAIN);
  
  /* Device Label */
  lv_obj_t *device_label = lv_label_create(scr);
  lv_label_set_text(device_label, "Device:");
  lv_obj_align(device_label, LV_ALIGN_TOP_LEFT, 5, 90);
  lv_obj_set_style_text_color(device_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_style_text_font(device_label, &lv_font_montserrat_12, LV_PART_MAIN);
  
  /* Device Value */
  lv_obj_t *device_val = lv_label_create(scr);
  lv_label_set_text(device_val, "HPH");
  lv_obj_align(device_val, LV_ALIGN_TOP_RIGHT, -5, 90);
  lv_obj_set_style_text_color(device_val, lv_color_hex(0xFFFF00), LV_PART_MAIN);
  lv_obj_set_style_text_font(device_val, &lv_font_montserrat_12, LV_PART_MAIN);
  
  /* Sample Rate Label */
  lv_obj_t *sample_rate_label = lv_label_create(scr);
  lv_label_set_text(sample_rate_label, "48kHz");
  lv_obj_align(sample_rate_label, LV_ALIGN_TOP_LEFT, 5, 110);
  lv_obj_set_style_text_color(sample_rate_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_style_text_font(sample_rate_label, &lv_font_montserrat_12, LV_PART_MAIN);
  
  /* Volume Label */
  lv_obj_t *volume_label = lv_label_create(scr);
  lv_label_set_text(volume_label, "Vol:");
  lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 5, 130);
  lv_obj_set_style_text_color(volume_label, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_12, LV_PART_MAIN);
  
  /* Volume Value */
  lv_obj_t *volume_val = lv_label_create(scr);
  lv_label_set_text(volume_val, "100%");
  lv_obj_align(volume_val, LV_ALIGN_TOP_RIGHT, -5, 130);
  lv_obj_set_style_text_color(volume_val, lv_color_hex(0xFFFF00), LV_PART_MAIN);
  lv_obj_set_style_text_font(volume_val, &lv_font_montserrat_12, LV_PART_MAIN);
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
/* USER CODE BEGIN 5 */
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

void StartAudioTask(void const * argument)
{
  /* USER CODE BEGIN StartAudioTask */
  UNUSED(argument);

  for(;;)
  {
    AUDIO_ServiceTaskStep();
    osDelay(1);
  }
  /* USER CODE END StartAudioTask */
}

void StartLcdTask(void const * argument)
{
  /* USER CODE BEGIN StartLcdTask */
  UNUSED(argument);

  /* Initialize LVGL and the display */
  lv_init();
  lv_port_disp_init();

  /* Create USB Audio Information UI */
  create_audio_info_ui();

  /* Main loop: handle LVGL tasks with sufficient delay to avoid blocking audio */
  for(;;)
  {
    /* Handle LVGL tasks every 30ms (don't update too frequently) */
    lv_timer_handler();
    osDelay(30);
  }
  /* USER CODE END StartLcdTask */
}


/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
