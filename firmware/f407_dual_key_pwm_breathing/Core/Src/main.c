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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE BEGIN PTD */

/* 保存一路呼吸灯的运行状态 */
typedef struct
{
  uint8_t enabled;   /* 0：关闭；1：正在呼吸 */
  int32_t level;     /* 当前亮度，范围0～999 */
  int32_t step;      /* 当前变化方向和步长 */
} BreathLed_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE BEGIN PD */

/* PWM计数范围：0～999 */
#define PWM_MAX_COMPARE       999

/* 每次改变的亮度值，越大呼吸越快，但变化越不细腻 */
#define BREATH_STEP           5

/* 每隔5ms改变一次亮度 */
#define BREATH_INTERVAL_MS    5U

/* 按键软件消抖时间 */
#define KEY_DEBOUNCE_MS       30U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim10;
TIM_HandleTypeDef htim11;

/* USER CODE BEGIN PV */
/* USER CODE BEGIN PV */

/* 红灯和绿灯各自拥有独立状态 */
static BreathLed_t red_led =
{
  .enabled = 0U,
  .level = 0,
  .step = BREATH_STEP
};

static BreathLed_t green_led =
{
  .enabled = 0U,
  .level = 0,
  .step = BREATH_STEP
};

/* 保存按键上一次的电平，用于检测按下沿 */
static GPIO_PinState key1_last = GPIO_PIN_RESET;
static GPIO_PinState key2_last = GPIO_PIN_RESET;

/* 保存最近一次有效按键事件时间，用于消抖 */
static uint32_t key1_last_event_ms = 0U;
static uint32_t key2_last_event_ms = 0U;

/* 上一次刷新呼吸亮度的时间 */
static uint32_t last_breath_ms = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM10_Init(void);
static void MX_TIM11_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE BEGIN PFP */

static uint8_t Key_GetPressEvent(
    GPIO_TypeDef *gpio_port,
    uint16_t gpio_pin,
    GPIO_PinState *last_state,
    uint32_t *last_event_ms);

static void BreathLed_Toggle(
    BreathLed_t *led,
    TIM_HandleTypeDef *htim);

static void BreathLed_Update(
    BreathLed_t *led,
    TIM_HandleTypeDef *htim);

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
  MX_TIM10_Init();
  MX_TIM11_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE BEGIN 2 */

/* 启动红灯和绿灯对应的PWM通道 */
if (HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1) != HAL_OK)
{
  Error_Handler();
}

if (HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1) != HAL_OK)
{
  Error_Handler();
}

/* 初始比较值为0，两盏灯保持熄灭 */
__HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, 0U);
__HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, 0U);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* USER CODE BEGIN WHILE */
while (1)
{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  uint32_t now = HAL_GetTick();

  /* KEY1每出现一次有效按下事件，就切换红灯状态 */
  if (Key_GetPressEvent(
          KEY1_GPIO_Port,
          KEY1_Pin,
          &key1_last,
          &key1_last_event_ms) != 0U)
  {
    BreathLed_Toggle(&red_led, &htim10);
  }

  /* KEY2每出现一次有效按下事件，就切换绿灯状态 */
  if (Key_GetPressEvent(
          KEY2_GPIO_Port,
          KEY2_Pin,
          &key2_last,
          &key2_last_event_ms) != 0U)
  {
    BreathLed_Toggle(&green_led, &htim11);
  }

  /*
   * 每5ms更新一次PWM占空比。
   * 不使用HAL_Delay，因此按键检测不会因为呼吸灯而停止。
   */
  if ((uint32_t)(now - last_breath_ms) >= BREATH_INTERVAL_MS)
  {
    last_breath_ms = now;

    BreathLed_Update(&red_led, &htim10);
    BreathLed_Update(&green_led, &htim11);
  }
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
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 167;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 999;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim10, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */
  HAL_TIM_MspPostInit(&htim10);

}

/**
  * @brief TIM11 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM11_Init(void)
{

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 167;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = 999;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim11, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */
  HAL_TIM_MspPostInit(&htim11);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : KEY2_Pin */
  GPIO_InitStruct.Pin = KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_B_Pin */
  GPIO_InitStruct.Pin = LED_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY1_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE BEGIN 4 */

/**
 * @brief 检测一次按键由低电平变为高电平的事件
 */
static uint8_t Key_GetPressEvent(
    GPIO_TypeDef *gpio_port,
    uint16_t gpio_pin,
    GPIO_PinState *last_state,
    uint32_t *last_event_ms)
{
  GPIO_PinState current_state;
  uint32_t now;
  uint8_t event = 0U;

  current_state = HAL_GPIO_ReadPin(gpio_port, gpio_pin);
  now = HAL_GetTick();

  /* 上一次为低，本次为高，说明刚刚按下 */
  if ((current_state == GPIO_PIN_SET) &&
      (*last_state == GPIO_PIN_RESET))
  {
    /* 两次有效事件至少间隔30ms，实现软件消抖 */
    if ((uint32_t)(now - *last_event_ms) >= KEY_DEBOUNCE_MS)
    {
      *last_event_ms = now;
      event = 1U;
    }
  }

  *last_state = current_state;

  return event;
}

/**
 * @brief 开启或关闭一路呼吸灯
 */
static void BreathLed_Toggle(
    BreathLed_t *led,
    TIM_HandleTypeDef *htim)
{
  led->enabled = !led->enabled;

  /* 每次切换都从完全熄灭、逐渐变亮开始 */
  led->level = 0;
  led->step = BREATH_STEP;

  __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, 0U);
}

/**
 * @brief 更新一路呼吸灯的亮度
 */
static void BreathLed_Update(
    BreathLed_t *led,
    TIM_HandleTypeDef *htim)
{
  /* 未启用时保持熄灭 */
  if (led->enabled == 0U)
  {
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, 0U);
    return;
  }

  /* 改变当前亮度 */
  led->level += led->step;

  /* 达到最亮后改为逐渐变暗 */
  if (led->level >= PWM_MAX_COMPARE)
  {
    led->level = PWM_MAX_COMPARE;
    led->step = -BREATH_STEP;
  }
  /* 达到最暗后改为逐渐变亮 */
  else if (led->level <= 0)
  {
    led->level = 0;
    led->step = BREATH_STEP;
  }

  /* 把亮度写入定时器比较寄存器，改变PWM占空比 */
  __HAL_TIM_SET_COMPARE(
      htim,
      TIM_CHANNEL_1,
      (uint32_t)led->level);
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
