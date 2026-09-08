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
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  PACKET_WAIT_HEADER = 0,
  PACKET_RECEIVE_PAYLOAD
} PacketParserState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_RX_FIFO_SIZE 256U
#define PACKET_PAYLOAD_MAX_SIZE 256U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* HAL每次接收一个字节 */
static uint8_t uart_rx_byte = 0U;

/* 串口接收环形FIFO */
static volatile uint8_t uart_rx_fifo[UART_RX_FIFO_SIZE];

/* 中断写入位置 */
static volatile uint16_t uart_rx_head = 0U;

/* 主循环读取位置 */
static volatile uint16_t uart_rx_tail = 0U;

/* FIFO满时的丢包计数 */
static volatile uint32_t uart_rx_overflow = 0U;
static uint32_t uart_rx_overflow_reported = 0U;

/* 串口数据包解析状态 */
static PacketParserState_t packet_state = PACKET_WAIT_HEADER;
static uint8_t packet_header_z_seen = 0U;
static uint8_t packet_tail_f_seen = 0U;
static uint8_t packet_payload[PACKET_PAYLOAD_MAX_SIZE];
static uint16_t packet_payload_length = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void UART_SendString(const char *text);
static void UART_ProtocolProcessByte(uint8_t data);
static void UART_PacketAppendByte(uint8_t data);
static void UART_PacketComplete(void);
static void UART_PacketReset(void);
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
UART_SendString(
    "\r\n"
    "STM32F407 UART packet receiver started.\r\n"
    "ASCII header = ZZ, ASCII tail = FF.\r\n"
    "Example: ZZ a f e c g d FF\r\n"
    "A complete packet prints only its payload.\r\n\r\n");

/*
 * 启动USART1中断接收。
 * 每次接收1个字节。
 */
if (HAL_UART_Receive_IT(
        &huart1,
        &uart_rx_byte,
        1U) != HAL_OK)
{
    Error_Handler();
}

/* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
{
    uint8_t data;
    uint8_t data_available = 0U;

    /*
     * 从FIFO取出收到的数据。
     */
    __disable_irq();

    if (uart_rx_tail != uart_rx_head)
    {
        data = uart_rx_fifo[uart_rx_tail];

        uart_rx_tail =
            (uint16_t)((uart_rx_tail + 1U) %
                       UART_RX_FIFO_SIZE);

        data_available = 1U;
    }

    __enable_irq();

    /* 将收到的字节交给数据包状态机处理。 */
    if (data_available != 0U)
    {
        UART_ProtocolProcessByte(data);
    }

    /* FIFO发生溢出时给出明确提示，便于区分协议错误和串口丢字节。 */
    if (uart_rx_overflow != uart_rx_overflow_reported)
    {
        uart_rx_overflow_reported = uart_rx_overflow;
        UART_SendString(
            "\r\nUART RX FIFO overflow: input was too fast.\r\n");
    }

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
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief USART接收完成回调
 *
 * 每收到一个字节，HAL库就会调用一次该函数。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t next_head;

    if (huart->Instance == USART1)
    {
        next_head =
            (uint16_t)((uart_rx_head + 1U) %
                       UART_RX_FIFO_SIZE);

        /*
         * next_head等于tail表示FIFO已经满了。
         */
        if (next_head != uart_rx_tail)
        {
            uart_rx_fifo[uart_rx_head] =
                uart_rx_byte;

            uart_rx_head = next_head;
        }
        else
        {
            uart_rx_overflow++;
        }

        /*
         * 立即启动下一字节接收。
         */
        if (HAL_UART_Receive_IT(
                &huart1,
                &uart_rx_byte,
                1U) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

/**
 * @brief 逐字节解析格式为 ZZ<payload>FF 的ASCII数据包
 *
 * 包头是连续两个大写字母Z，包尾是连续两个大写字母F。
 * 单独出现的F仍然属于payload；连续两个F才结束数据包。
 */
static void UART_ProtocolProcessByte(uint8_t data)
{
    if (packet_state == PACKET_WAIT_HEADER)
    {
        if (data == (uint8_t)'Z')
        {
            if (packet_header_z_seen != 0U)
            {
                packet_state = PACKET_RECEIVE_PAYLOAD;
                packet_header_z_seen = 0U;
                packet_tail_f_seen = 0U;
                packet_payload_length = 0U;
            }
            else
            {
                packet_header_z_seen = 1U;
            }
        }
        else
        {
            packet_header_z_seen = 0U;
        }

        return;
    }

    /* 已经找到包头，现在寻找连续两个F作为包尾。 */
    if (data == (uint8_t)'F')
    {
        if (packet_tail_f_seen != 0U)
        {
            UART_PacketComplete();
            UART_PacketReset();
        }
        else
        {
            packet_tail_f_seen = 1U;
        }

        return;
    }

    /* 前一个F后面不是F，因此前一个F属于payload。 */
    if (packet_tail_f_seen != 0U)
    {
        UART_PacketAppendByte((uint8_t)'F');
        packet_tail_f_seen = 0U;
    }

    UART_PacketAppendByte(data);
}

/**
 * @brief 向payload数组追加一个字节，并处理超长数据包
 */
static void UART_PacketAppendByte(uint8_t data)
{
    if (packet_payload_length < PACKET_PAYLOAD_MAX_SIZE)
    {
        packet_payload[packet_payload_length] = data;
        packet_payload_length++;
    }
    else
    {
        UART_SendString("\r\nPacket error: payload too long.\r\n");
        UART_PacketReset();
    }
}

/**
 * @brief 完整收到一个数据包后，只显示中间payload
 */
static void UART_PacketComplete(void)
{
    uint16_t first = 0U;
    uint16_t last = packet_payload_length;

    /* 去掉payload开头和结尾用于分隔的空格。 */
    while ((first < last) &&
           ((packet_payload[first] == (uint8_t)' ') ||
            (packet_payload[first] == (uint8_t)'\t')))
    {
        first++;
    }

    while ((last > first) &&
           ((packet_payload[last - 1U] == (uint8_t)' ') ||
            (packet_payload[last - 1U] == (uint8_t)'\t')))
    {
        last--;
    }

    UART_SendString("\r\nPacket received.\r\nRX DATA: ");

    if (last > first)
    {
        HAL_UART_Transmit(
            &huart1,
            &packet_payload[first],
            (uint16_t)(last - first),
            100U);
    }
    else
    {
        UART_SendString("<empty>");
    }

    UART_SendString("\r\n");
}

/**
 * @brief 恢复到等待下一个ZZ包头的状态
 */
static void UART_PacketReset(void)
{
    packet_state = PACKET_WAIT_HEADER;
    packet_header_z_seen = 0U;
    packet_tail_f_seen = 0U;
    packet_payload_length = 0U;
}

/**
 * @brief 通过USART1发送字符串
 */
static void UART_SendString(const char *text)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)text,
        (uint16_t)strlen(text),
        100U);
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
