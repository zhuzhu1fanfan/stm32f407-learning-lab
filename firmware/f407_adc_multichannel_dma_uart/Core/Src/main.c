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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ADC扫描通道数量 */
#define ADC_CHANNEL_COUNT       3U

/* DMA数组下标必须与ADC Rank顺序一致 */
#define ADC_INDEX_POTENTIOMETER 0U
#define ADC_INDEX_TEMPERATURE   1U
#define ADC_INDEX_VREFINT       2U

/* VREFINT典型电压，单位mV */
#define VREFINT_TYPICAL_MV      1210U

/* 内部温度传感器25℃时典型电压，单位μV */
#define TEMP_V25_UV             760000LL

/* 平均斜率约2.5mV/℃，即2500μV/℃ */
#define TEMP_AVG_SLOPE_UV       2500LL
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/*
 * DMA直接写入该数组。
 * Word数据宽度对应uint32_t。
 */
static uint32_t adc_dma_buffer[ADC_CHANNEL_COUNT] = {0U};

/* DMA完成一次三通道扫描后，将数据复制到这里 */
static volatile uint32_t adc_latest[ADC_CHANNEL_COUNT] = {0U};

/* 表示DMA已经获得一组完整数据 */
static volatile uint8_t adc_data_ready = 0U;

/* 方便在调试器Watch窗口中观察 */
static volatile uint32_t pot_raw = 0U;
static volatile uint32_t temperature_raw = 0U;
static volatile uint32_t vrefint_raw = 0U;

static volatile uint32_t pot_voltage_mv = 0U;
static volatile uint32_t estimated_vdda_mv = 3300U;
static volatile int32_t temperature_x10 = 0;

static uint32_t last_uart_ms = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void UART_LogAdcData(
    uint32_t pot_adc,
    uint32_t pot_mv,
    uint32_t temp_adc,
    int32_t temp_c_x10,
    uint32_t vref_adc,
    uint32_t vdda_mv);
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
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  {
  const char start_message[] =
      "ADC multichannel DMA test started.\r\n"
      "Rank1=Pot, Rank2=Temperature, Rank3=VREFINT\r\n\r\n";

  HAL_UART_Transmit(
      &huart1,
      (uint8_t *)start_message,
      sizeof(start_message) - 1U,
      100U);
}

/*
 * 等待内部温度传感器和VREFINT稳定。
 * 1ms远大于内部通道所需启动时间。
 */
HAL_Delay(1);

/*
 * 启动ADC DMA。
 * DMA每次按照Rank顺序搬运3个32位数据。
 */
if (HAL_ADC_Start_DMA(
        &hadc1,
        adc_dma_buffer,
        ADC_CHANNEL_COUNT) != HAL_OK)
{
  Error_Handler();
}

/*
 * ADC准备完成后再启动TIM2。
 * TIM2每10ms产生一次TRGO，触发ADC扫描三个通道。
 */
if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
{
  Error_Handler();
}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
      /*
 * DMA每10ms更新一次数据。
 * 串口每200ms输出一次，避免刷新过快。
 */
if ((adc_data_ready != 0U) &&
    ((HAL_GetTick() - last_uart_ms) >= 2000U))
{
  uint32_t local_pot;
  uint32_t local_temp;
  uint32_t local_vref;

  int64_t temperature_voltage_uv;
  int64_t temperature_delta_x10;

  last_uart_ms = HAL_GetTick();

  /*
   * 暂时关闭中断，保证三个通道来自同一次完整快照。
   */
  __disable_irq();

  local_pot =
      adc_latest[ADC_INDEX_POTENTIOMETER];

  local_temp =
      adc_latest[ADC_INDEX_TEMPERATURE];

  local_vref =
      adc_latest[ADC_INDEX_VREFINT];

  adc_data_ready = 0U;

  __enable_irq();

  pot_raw = local_pot;
  temperature_raw = local_temp;
  vrefint_raw = local_vref;

  /*
   * 使用内部VREFINT估算当前VDDA：
   *
   * VDDA = VREFINT典型值 × 4095 / VREFINT_ADC
   *
   * 这是近似估算值，不是高精度校准结果。
   */
  if (local_vref != 0U)
  {
    estimated_vdda_mv =
        (VREFINT_TYPICAL_MV * 4095U +
         local_vref / 2U) /
        local_vref;
  }

  /*
   * 使用估算后的VDDA计算电位器电压。
   */
  pot_voltage_mv =
      (local_pot * estimated_vdda_mv +
       2047U) /
      4095U;

  /*
   * 将内部温度传感器ADC值转换成μV。
   * 使用int64_t防止乘法溢出。
   */
  temperature_voltage_uv =
      ((int64_t)local_temp *
       (int64_t)estimated_vdda_mv *
       1000LL) /
      4095LL;

  /*
   * 温度典型计算公式：
   *
   * Temperature =
   * (VSENSE - V25) / AvgSlope + 25℃
   *
   * temperature_x10使用0.1℃作为单位。
   */
  temperature_delta_x10 =
      ((temperature_voltage_uv -
        TEMP_V25_UV) * 10LL) /
      TEMP_AVG_SLOPE_UV;

  temperature_x10 =
      (int32_t)(250LL +
                temperature_delta_x10);

  UART_LogAdcData(
      pot_raw,
      pot_voltage_mv,
      temperature_raw,
      temperature_x10,
      vrefint_raw,
      estimated_vdda_mv);
}

/* 主循环不需要高速空转 */
HAL_Delay(1);
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
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 2;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief ADC DMA完成一次三通道扫描后的回调函数
 *
 * TIM2每10ms触发一次ADC。
 * ADC按照Rank 1、2、3依次转换。
 * DMA完成三个数据搬运后进入该函数。
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc_latest[ADC_INDEX_POTENTIOMETER] =
        adc_dma_buffer[ADC_INDEX_POTENTIOMETER];

    adc_latest[ADC_INDEX_TEMPERATURE] =
        adc_dma_buffer[ADC_INDEX_TEMPERATURE];

    adc_latest[ADC_INDEX_VREFINT] =
        adc_dma_buffer[ADC_INDEX_VREFINT];

    adc_data_ready = 1U;
  }
}

/**
 * @brief 将三通道ADC结果发送到电脑
 */
static void UART_LogAdcData(
    uint32_t pot_adc,
    uint32_t pot_mv,
    uint32_t temp_adc,
    int32_t temp_c_x10,
    uint32_t vref_adc,
    uint32_t vdda_mv)
{
  char message[220];
  int length;

  uint32_t temperature_absolute;
  const char *temperature_sign;

  if (temp_c_x10 < 0)
  {
    temperature_sign = "-";
    temperature_absolute =
        (uint32_t)(-temp_c_x10);
  }
  else
  {
    temperature_sign = "";
    temperature_absolute =
        (uint32_t)temp_c_x10;
  }

  length = snprintf(
      message,
      sizeof(message),
      "POT : ADC=%4lu, Voltage=%lu.%03lu V\r\n"
      "TEMP: ADC=%4lu, Approx=%s%lu.%01lu C\r\n"
      "VREF: ADC=%4lu, Estimated VDDA=%lu.%03lu V\r\n"
      "\r\n",
      (unsigned long)pot_adc,
      (unsigned long)(pot_mv / 1000U),
      (unsigned long)(pot_mv % 1000U),
      (unsigned long)temp_adc,
      temperature_sign,
      (unsigned long)(temperature_absolute / 10U),
      (unsigned long)(temperature_absolute % 10U),
      (unsigned long)vref_adc,
      (unsigned long)(vdda_mv / 1000U),
      (unsigned long)(vdda_mv % 1000U));

  if (length > 0)
  {
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)message,
        (uint16_t)length,
        100U);
  }
}
/* USER CODE END 4 */

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
