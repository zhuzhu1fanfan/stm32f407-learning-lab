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
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEY_DEBOUNCE_MS  50U
#define UART_TIMEOUT_MS  100U
/* 蜂鸣器每次鸣叫150ms */
#define BUZZER_DURATION_MS  150U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* 中断函数设置、主循环读取，所以使用volatile */
static volatile uint8_t key1_press_event = 0U;
static volatile uint8_t key2_press_event = 0U;

/* 两个按键各自的消抖时间 */
static volatile uint32_t key1_last_irq_ms = 0U;
static volatile uint32_t key2_last_irq_ms = 0U;

/* 当前计数值 */
static uint32_t key_count = 0U;
/* 蜂鸣器当前是否正在鸣叫 */
static uint8_t buzzer_on = 0U;

/* 本次鸣叫开始时间 */
static uint32_t buzzer_start_ms = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void Counter_Log(const char *event_name);
static void Buzzer_Set(uint8_t on);
static void Buzzer_Start(uint32_t now);
static void Buzzer_Update(uint32_t now);
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 启动时确保蜂鸣器关闭 */
Buzzer_Set(0U);

/* 上电后向电脑发送启动信息 */
Counter_Log("System started");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* USER CODE BEGIN WHILE */
while (1)
{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /*
   * KEY1中断只负责产生事件。
   * 计数和串口发送都在主循环中完成。
   */
  if (key1_press_event != 0U)
  {
    key1_press_event = 0U;

    key_count++;

    Counter_Log("KEY1 pressed");
    Buzzer_Start(HAL_GetTick());
  }

  /*
   * KEY2按下后清零。
   * 如果两个事件同时发生，KEY2最后处理，因此清零优先。
   */
  if (key2_press_event != 0U)
  {
    key2_press_event = 0U;

    key_count = 0U;

    Counter_Log("KEY2 pressed, counter cleared");
    Buzzer_Start(HAL_GetTick());
  }
  /* 非阻塞检查蜂鸣器是否已经响满150ms */
Buzzer_Update(HAL_GetTick());
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : KEY2_Pin */
  GPIO_InitStruct.Pin = KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY1_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief GPIO外部中断回调函数
 * @param GPIO_Pin 产生中断的GPIO引脚
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();

  /* KEY1：PA0，对应EXTI0 */
  if (GPIO_Pin == KEY1_Pin)
  {
    if ((uint32_t)(now - key1_last_irq_ms) >= KEY_DEBOUNCE_MS)
    {
      key1_last_irq_ms = now;
      key1_press_event = 1U;
    }
  }
  /* KEY2：PC13，对应EXTI13 */
  else if (GPIO_Pin == KEY2_Pin)
  {
    if ((uint32_t)(now - key2_last_irq_ms) >= KEY_DEBOUNCE_MS)
    {
      key2_last_irq_ms = now;
      key2_press_event = 1U;
    }
  }
}

/**
 * @brief 把事件名称和当前计数值发送到电脑
 */
static void Counter_Log(const char *event_name)
{
  char message[100];
  int length;

  /*
   * snprintf把字符串和计数值组合到message数组中。
   * %lu用于输出unsigned long。
   */
  length = snprintf(
      message,
      sizeof(message),
      "%s, Count = %lu\r\n",
      event_name,
      (unsigned long)key_count);

  if (length > 0)
  {
    /*
     * snprintf返回值可能大于缓冲区长度，
     * 因此对实际发送长度做限制。
     */
    uint16_t transmit_length =
        (length < (int)sizeof(message))
            ? (uint16_t)length
            : (uint16_t)(sizeof(message) - 1U);

    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)message,
        transmit_length,
        UART_TIMEOUT_MS);
  }
}
/**
 * @brief 控制板载蜂鸣器
 * @param on 1：响；0：关闭
 */
static void Buzzer_Set(uint8_t on)
{
  /*
   * 霸天虎板载蜂鸣器PG7高电平有效：
   * 高电平响，低电平关闭。
   */
  HAL_GPIO_WritePin(
      BUZZER_GPIO_Port,
      BUZZER_Pin,
      (on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 开始一次蜂鸣
 * @param now 当前HAL毫秒计数
 */
static void Buzzer_Start(uint32_t now)
{
  buzzer_on = 1U;
  buzzer_start_ms = now;

  Buzzer_Set(1U);
}

/**
 * @brief 非阻塞更新蜂鸣器状态
 * @param now 当前HAL毫秒计数
 */
static void Buzzer_Update(uint32_t now)
{
  if ((buzzer_on != 0U) &&
      ((uint32_t)(now - buzzer_start_ms) >= BUZZER_DURATION_MS))
  {
    buzzer_on = 0U;

    Buzzer_Set(0U);
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
