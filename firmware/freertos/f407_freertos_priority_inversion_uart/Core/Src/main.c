/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : FreeRTOS three-car priority inversion demonstration
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  char text[192];
} LogMessage;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CAR1_PRIORITY       ((UBaseType_t)1U)
#define CAR2_PRIORITY       ((UBaseType_t)2U)
#define CAR3_PRIORITY       ((UBaseType_t)3U)
#define SUPERVISOR_PRIORITY ((UBaseType_t)4U)
#define LOGGER_PRIORITY     ((UBaseType_t)5U)

#define CAR1_DRIVE_MS       2000U
#define CAR2_START_MS        800U
#define CAR2_DRIVE_MS       3000U
#define CAR3_START_MS        500U
#define CAR3_DRIVE_MS        150U
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static QueueHandle_t log_queue = NULL;
static SemaphoreHandle_t road_lock = NULL;
static TaskHandle_t supervisor_task_handle = NULL;
static TaskHandle_t car2_task_handle = NULL;
static TaskHandle_t car3_task_handle = NULL;
static volatile uint32_t car3_wait_ms = 0U;
static volatile uint32_t busy_sink = 0U;
static uint8_t use_mutex = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void UART_SendDirect(const char *text);
static void Log_Printf(const char *format, ...);
static void BusyForMilliseconds(uint32_t duration_ms);
static void LoggerTask(void *argument);
static void SupervisorTask(void *argument);
static void Car1LowTask(void *argument);
static void Car2MediumTask(void *argument);
static void Car3HighTask(void *argument);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static void UART_SendDirect(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)strlen(text), 500U);
}

static void Log_Printf(const char *format, ...)
{
  LogMessage message;
  va_list arguments;

  va_start(arguments, format);
  (void)vsnprintf(message.text, sizeof(message.text), format, arguments);
  va_end(arguments);

  if (log_queue != NULL)
  {
    (void)xQueueSend(log_queue, &message, pdMS_TO_TICKS(100U));
  }
}

static void BusyForMilliseconds(uint32_t duration_ms)
{
  const TickType_t start = xTaskGetTickCount();
  const TickType_t duration_ticks = pdMS_TO_TICKS(duration_ms);

  /*
   * 故意不调用vTaskDelay()。任务始终处于Ready/Running状态，
   * 才能清楚演示中优先级任务抢占低优先级任务的效果。
   */
  while ((TickType_t)(xTaskGetTickCount() - start) < duration_ticks)
  {
    busy_sink++;
    __NOP();
  }
}

static void LoggerTask(void *argument)
{
  LogMessage message;
  (void)argument;

  for (;;)
  {
    if (xQueueReceive(log_queue, &message, portMAX_DELAY) == pdPASS)
    {
      HAL_UART_Transmit(&huart1, (uint8_t *)message.text,
                        (uint16_t)strlen(message.text), 500U);
    }
  }
}

static void Car1LowTask(void *argument)
{
  const TickType_t phase_start = xTaskGetTickCount();
  (void)argument;

  (void)xSemaphoreTake(road_lock, portMAX_DELAY);
  Log_Printf("[%4lu ms] Car1 LOW entered the one-lane bridge, priority=%lu.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start),
             (unsigned long)uxTaskPriorityGet(NULL));
  Log_Printf("ROAD: [Car1 >>>>>>>] [Car2 ready] [Car3 ready]\r\n");

  /*
   * Car2与Car3确认Car1占用共享道路后才开始计时，
   * 使两次对照实验具有确定性。
   */
  xTaskNotifyGive(car2_task_handle);
  xTaskNotifyGive(car3_task_handle);

  BusyForMilliseconds(CAR1_DRIVE_MS);

  Log_Printf("[%4lu ms] Car1 finishing; effective priority=%lu.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start),
             (unsigned long)uxTaskPriorityGet(NULL));
  Log_Printf("[%4lu ms] Car1 LOW leaves the bridge.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start));

  (void)xSemaphoreGive(road_lock);
  xTaskNotifyGive(supervisor_task_handle);
  vTaskDelete(NULL);
}

static void Car2MediumTask(void *argument)
{
  const TickType_t phase_start = xTaskGetTickCount();
  (void)argument;

  (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(CAR2_START_MS));

  Log_Printf("[%4lu ms] Car2 MEDIUM starts CPU-heavy driving.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start));
  Log_Printf("ROAD: [Car1 owns bridge] [Car2 >>>>>>>] [Car3 waiting]\r\n");

  BusyForMilliseconds(CAR2_DRIVE_MS);

  Log_Printf("[%4lu ms] Car2 MEDIUM finishes.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start));

  xTaskNotifyGive(supervisor_task_handle);
  vTaskDelete(NULL);
}

static void Car3HighTask(void *argument)
{
  TickType_t wait_start;
  const TickType_t phase_start = xTaskGetTickCount();
  (void)argument;

  (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  vTaskDelay(pdMS_TO_TICKS(CAR3_START_MS));

  wait_start = xTaskGetTickCount();
  Log_Printf("[%4lu ms] Car3 HIGH arrives and waits for the bridge.\r\n",
             (unsigned long)(wait_start - phase_start));

  (void)xSemaphoreTake(road_lock, portMAX_DELAY);
  car3_wait_ms = (uint32_t)((xTaskGetTickCount() - wait_start) * portTICK_PERIOD_MS);

  Log_Printf("[%4lu ms] Car3 HIGH enters; waited %lu ms.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start),
             (unsigned long)car3_wait_ms);
  Log_Printf("ROAD: [Car1 done] [Car2 state depends on mode] [Car3 >>>>>>>]\r\n");

  BusyForMilliseconds(CAR3_DRIVE_MS);
  (void)xSemaphoreGive(road_lock);

  Log_Printf("[%4lu ms] Car3 HIGH leaves the bridge.\r\n",
             (unsigned long)(xTaskGetTickCount() - phase_start));

  xTaskNotifyGive(supervisor_task_handle);
  vTaskDelete(NULL);
}

static void SupervisorTask(void *argument)
{
  (void)argument;

  vTaskDelay(pdMS_TO_TICKS(500U));

  for (;;)
  {
    car3_wait_ms = 0U;

    if (use_mutex == 0U)
    {
      road_lock = xSemaphoreCreateBinary();
      configASSERT(road_lock != NULL);
      (void)xSemaphoreGive(road_lock);

      Log_Printf("\r\n================================================\r\n");
      Log_Printf("DEMO A: binary semaphore - NO priority inheritance\r\n");
      Log_Printf("Priority: Car1=LOW(1), Car2=MEDIUM(2), Car3=HIGH(3)\r\n");
      Log_Printf("Expected: Car2 preempts Car1; Car3 waits much longer.\r\n\r\n");
    }
    else
    {
      road_lock = xSemaphoreCreateMutex();
      configASSERT(road_lock != NULL);

      Log_Printf("\r\n================================================\r\n");
      Log_Printf("DEMO B: mutex - priority inheritance ENABLED\r\n");
      Log_Printf("Priority: Car1=LOW(1), Car2=MEDIUM(2), Car3=HIGH(3)\r\n");
      Log_Printf("Expected: Car1 temporarily inherits priority 3.\r\n\r\n");
    }

    configASSERT(xTaskCreate(Car2MediumTask, "Car2Medium", 256U, NULL,
                             CAR2_PRIORITY, &car2_task_handle) == pdPASS);
    configASSERT(xTaskCreate(Car3HighTask, "Car3High", 256U, NULL,
                             CAR3_PRIORITY, &car3_task_handle) == pdPASS);
    configASSERT(xTaskCreate(Car1LowTask, "Car1Low", 256U, NULL,
                             CAR1_PRIORITY, NULL) == pdPASS);

    /* Wait until all three cars finish this phase. */
    for (uint32_t finished = 0U; finished < 3U; finished++)
    {
      (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(12000U));
    }

    if (use_mutex == 0U)
    {
      Log_Printf("\r\nRESULT A: Car3 waited %lu ms -> priority inversion.\r\n",
                 (unsigned long)car3_wait_ms);
    }
    else
    {
      Log_Printf("\r\nRESULT B: Car3 waited %lu ms -> inheritance protects Car3.\r\n",
                 (unsigned long)car3_wait_ms);
    }

    vSemaphoreDelete(road_lock);
    road_lock = NULL;

    use_mutex = (uint8_t)(use_mutex == 0U);
    Log_Printf("Next comparison starts after 3 seconds.\r\n");
    vTaskDelay(pdMS_TO_TICKS(3000U));
  }
}

void vApplicationTickHook(void)
{
  HAL_IncTick();
}

void vApplicationMallocFailedHook(void)
{
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
  (void)task;
  (void)task_name;
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
  }
}
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  UART_SendDirect("\r\nSTM32F407 FreeRTOS three-car scheduler demo.\r\n");
  UART_SendDirect("USART1=115200 8-N-1. Starting scheduler...\r\n");

  log_queue = xQueueCreate(24U, sizeof(LogMessage));
  configASSERT(log_queue != NULL);

  configASSERT(xTaskCreate(LoggerTask, "Logger", 384U, NULL,
                           LOGGER_PRIORITY, NULL) == pdPASS);
  configASSERT(xTaskCreate(SupervisorTask, "Supervisor", 384U, NULL,
                           SUPERVISOR_PRIORITY,
                           &supervisor_task_handle) == pdPASS);

  vTaskStartScheduler();

  /* The scheduler returns only if the FreeRTOS heap is insufficient. */
  Error_Handler();
  /* USER CODE END 2 */

  while (1)
  {
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                               RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void)
{
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
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
