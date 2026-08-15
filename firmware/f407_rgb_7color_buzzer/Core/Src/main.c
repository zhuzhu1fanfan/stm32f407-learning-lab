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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/*
 * 以下时间参数的单位都是毫秒，由 HAL_Delay() 使用。
 * 使用宏集中管理参数，调整灯效时不必修改主循环的控制逻辑。
 * 整数字面量后的 U 表示 unsigned，和 HAL 的 uint32_t 参数类型一致。
 */
#define COLOR_DELAY_MS              200U  /* 每种颜色保持 200 ms */
#define COLOR_TO_BUZZER_GAP_MS      100U  /* 熄灯后等待一小段时间再蜂鸣 */
#define BUZZER_DURATION_MS          200U  /* 蜂鸣器持续响 200 ms */
#define ROUND_INTERVAL_MS           500U  /* 一轮结束到下一轮开始的间隔 */

/*
 * 用 uint32_t 的低三位表示红、绿、蓝三个逻辑通道：
 * bit0 = 红，bit1 = 绿，bit2 = 蓝。
 * 1U << n 表示把无符号整数 1 左移 n 位，得到互不重叠的位掩码。
 */
#define COLOR_OFF          0U
#define COLOR_RED          (1U << 0)
#define COLOR_GREEN        (1U << 1)
#define COLOR_BLUE         (1U << 2)

/*
 * 按位或运算符 | 可以同时置位多个颜色通道。
 * 例如 COLOR_RED | COLOR_GREEN 的二进制值是 011，即红、绿同时点亮。
 */
#define COLOR_YELLOW       (COLOR_RED | COLOR_GREEN)
#define COLOR_PURPLE       (COLOR_RED | COLOR_BLUE)
#define COLOR_CYAN         (COLOR_GREEN | COLOR_BLUE)
#define COLOR_WHITE        (COLOR_RED | COLOR_GREEN | COLOR_BLUE)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/*
 * 颜色播放表：把“播放顺序”与“播放方法”分开。
 * static：该数组只在本 main.c 中可见；
 * const：运行期间只读，防止代码意外修改颜色表。
 */
static const uint32_t color_sequence[] =
{
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_BLUE,
  COLOR_PURPLE,
  COLOR_WHITE
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
/* 根据颜色位掩码，一次更新 RGB 三个通道的亮灭状态。 */
static void RGB_SetColor(uint32_t color);

/* 让板载蜂鸣器响指定的毫秒数，然后自动关闭。 */
static void Buzzer_Beep(uint32_t duration_ms);
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /*
     * sizeof(整个数组) / sizeof(一个元素) 可以得到数组元素个数。
     * 这样以后增加或删除颜色时，不需要手动修改循环上限。
     */
    for (uint32_t i = 0U;
         i < sizeof(color_sequence) / sizeof(color_sequence[0]);
         i++)
    {
      /* 读取颜色表中的第 i 个颜色，并更新三个 GPIO。 */
      RGB_SetColor(color_sequence[i]);

      /* 阻塞保持当前颜色；本项目简单，暂时允许使用阻塞延时。 */
      HAL_Delay(COLOR_DELAY_MS);
    }

    /* 一轮颜色结束后关闭 RGB，避免蜂鸣时仍保持白色。 */
    RGB_SetColor(COLOR_OFF);
    HAL_Delay(COLOR_TO_BUZZER_GAP_MS);

    /* 用声音提示七种颜色已经完整播放一轮。 */
    Buzzer_Beep(BUZZER_DURATION_MS);

    /* 保留轮次间隔，避免蜂鸣声连续得难以分辨。 */
    HAL_Delay(ROUND_INTERVAL_MS);
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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, LED1_Pin|LED2_Pin|LED3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED1_Pin LED2_Pin LED3_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin|LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  根据逻辑颜色值控制板载 RGB LED。
  * @param  color 由 COLOR_RED/GREEN/BLUE 按位组合成的颜色掩码。
  * @retval None
  *
  * 板载 RGB LED 为低电平有效：GPIO 输出低电平时点亮，输出高电平时熄灭。
  * 因此这里把“逻辑上选中某颜色”转换成 GPIO_PIN_RESET。
  */
static void RGB_SetColor(uint32_t color)
{
  /*
   * color & COLOR_RED 用按位与检查红色位：
   * 结果非 0 表示需要点亮红灯，结果为 0 表示需要熄灭红灯。
   * 条件运算符“条件 ? 值1 : 值2”根据检查结果选择输出电平。
   */
  HAL_GPIO_WritePin(
      LED1_GPIO_Port,
      LED1_Pin,
      (color & COLOR_RED) ? GPIO_PIN_RESET : GPIO_PIN_SET);

  /* 使用同样的方法检查绿色位并控制 PF7。 */
  HAL_GPIO_WritePin(
      LED2_GPIO_Port,
      LED2_Pin,
      (color & COLOR_GREEN) ? GPIO_PIN_RESET : GPIO_PIN_SET);

  /* 使用同样的方法检查蓝色位并控制 PF8。 */
  HAL_GPIO_WritePin(
      LED3_GPIO_Port,
      LED3_Pin,
      (color & COLOR_BLUE) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
  * @brief  产生一次固定时长的蜂鸣提示。
  * @param  duration_ms 蜂鸣持续时间，单位为毫秒。
  * @retval None
  *
  * 霸天虎 V2 的蜂鸣器由 PG7 高电平使能、低电平关闭。
  * 函数退出前主动恢复低电平，保证调用结束后蜂鸣器保持安静。
  */
static void Buzzer_Beep(uint32_t duration_ms)
{
  /* PG7 置高，打开板载蜂鸣器。 */
  HAL_GPIO_WritePin(
      BUZZER_GPIO_Port,
      BUZZER_Pin,
      GPIO_PIN_SET);

  /* 保持蜂鸣 duration_ms 毫秒；这是阻塞延时。 */
  HAL_Delay(duration_ms);

  /* PG7 恢复低电平，关闭蜂鸣器，避免持续鸣叫。 */
  HAL_GPIO_WritePin(
      BUZZER_GPIO_Port,
      BUZZER_Pin,
      GPIO_PIN_RESET);
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
