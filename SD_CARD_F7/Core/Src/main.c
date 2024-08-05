/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BUFFER_SIZE 128 * 1024

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SD_HandleTypeDef  hsd1;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;

/* USER CODE BEGIN PV */

/*
 * Variables for the SD card
 */
FRESULT fr = FR_NOT_READY;
FATFS   SDFatFs_main; /* File system object for SD card logical drive */
FIL     MyFile;       /* File object */

char    buffer[BUFFER_SIZE]; // to store strings..
UINT    br, bw;
uint8_t rec_char[20];

uint32_t total_size    = 1UL * 1024 * 1024 * 1024; // 1GB
uint32_t bytes_written = 0;

uint32_t time_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void        SystemClock_Config(void);
void        PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SDMMC1_SD_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int
bufsize (char *buf)
{
  int i = 0;
  while (*buf++ != '\0')
  {
    i++;
  }
  return i;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int
main (void)
{

  /* USER CODE BEGIN 1 */
  memset(buffer, 0xFF, BUFFER_SIZE); // Fill buffer with data
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FATFS_Init();
  MX_SDMMC1_SD_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
  {
    Error_Handler();
  }

  __disable_irq();
  time_count = 0;
  __enable_irq();
  fr = f_mount(&SDFatFs_main, "", 1);

  fr = f_open(&MyFile, "file3.txt", FA_CREATE_ALWAYS | FA_WRITE);

  /* Writing text */
  while (bytes_written < total_size)
  {
    fr = f_write(&MyFile, buffer, bufsize(buffer), &bw);

    bytes_written += bw;
  }
  f_close(&MyFile);

  __disable_irq();
  uint32_t temp = time_count;
  __enable_irq();

  /* USER CODE END 2 */

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
void
SystemClock_Config (void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_7);
  while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_7)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_PWR_EnableOverDriveMode();
  LL_RCC_HSE_EnableBypass();
  LL_RCC_HSE_Enable();

  /* Wait till HSE is ready */
  while (LL_RCC_HSE_IsReady() != 1)
  {
  }
  LL_RCC_PLL_ConfigDomain_SYS(
      LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 432, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 1)
  {
  }
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

  /* Wait till System clock is ready */
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }
  LL_SetSystemCoreClock(216000000);

  /* Update the time base */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }

  /* Set Timers Clock Prescalers */
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void
PeriphCommonClock_Config (void)
{
  LL_RCC_PLLSAI_ConfigDomain_48M(
      LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_25, 192, LL_RCC_PLLSAIP_DIV_4);
  LL_RCC_PLLSAI_Enable();

  /* Wait till PLL is ready */
  while (LL_RCC_PLLSAI_IsReady() != 1)
  {
  }
}

/**
 * @brief SDMMC1 Initialization Function
 * @param None
 * @retval None
 */
static void
MX_SDMMC1_SD_Init (void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance                 = SDMMC1;
  hsd1.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass         = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide             = SDMMC_BUS_WIDE_1B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv            = 0x0;
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void
MX_DMA_Init (void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void
MX_GPIO_Init (void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);

  /**/
  GPIO_InitStruct.Pin  = LL_GPIO_PIN_13;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/* MPU Configuration */

void
MPU_Config (void)
{

  /* Disables the MPU */
  LL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
   */
  LL_MPU_ConfigRegion(
      LL_MPU_REGION_NUMBER0,
      0x87,
      0x0,
      LL_MPU_REGION_SIZE_4GB | LL_MPU_TEX_LEVEL0 | LL_MPU_REGION_NO_ACCESS
          | LL_MPU_INSTRUCTION_ACCESS_DISABLE | LL_MPU_ACCESS_SHAREABLE
          | LL_MPU_ACCESS_NOT_CACHEABLE | LL_MPU_ACCESS_NOT_BUFFERABLE);
  /* Enables the MPU */
  LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void
Error_Handler (void)
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
void
assert_failed (uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
