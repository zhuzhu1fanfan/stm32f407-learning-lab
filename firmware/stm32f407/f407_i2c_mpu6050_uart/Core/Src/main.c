/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : I2C1 MPU6050 acquisition and USART1 display
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  int16_t ax_raw;
  int16_t ay_raw;
  int16_t az_raw;
  int16_t temp_raw;
  int16_t gx_raw;
  int16_t gy_raw;
  int16_t gz_raw;
} MPU6050_RawData;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MPU6050_ADDR_LOW       (0x68U << 1)
#define MPU6050_ADDR_HIGH      (0x69U << 1)

#define MPU6050_REG_SMPLRT_DIV 0x19U
#define MPU6050_REG_CONFIG     0x1AU
#define MPU6050_REG_GYRO_CFG   0x1BU
#define MPU6050_REG_ACCEL_CFG  0x1CU
#define MPU6050_REG_ACCEL_XOUT 0x3BU
#define MPU6050_REG_PWR_MGMT_1 0x6BU
#define MPU6050_REG_WHO_AM_I   0x75U

#define MPU6050_WHO_AM_I_VALUE 0x68U
#define RAD_TO_DEG              57.2957795f
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static uint16_t mpu6050_address = MPU6050_ADDR_LOW;
static int32_t gyro_x_bias = 0;
static int32_t gyro_y_bias = 0;
static int32_t gyro_z_bias = 0;
static float roll_deg = 0.0f;
static float pitch_deg = 0.0f;
static float yaw_deg = 0.0f;
static uint8_t angle_initialized = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void UART_SendString(const char *text);
static void FormatFixed100(char *text, size_t text_size, int32_t value_x100);
static HAL_StatusTypeDef MPU6050_WriteRegister(uint8_t reg, uint8_t value);
static HAL_StatusTypeDef MPU6050_ReadRegisters(uint8_t reg, uint8_t *data,
                                               uint16_t length);
static HAL_StatusTypeDef MPU6050_Init(void);
static HAL_StatusTypeDef MPU6050_ReadRaw(MPU6050_RawData *sample);
static void MPU6050_CalibrateGyroscope(void);
static void MPU6050_UpdateAngles(const MPU6050_RawData *sample, float dt_s);
static void MPU6050_PrintSample(const MPU6050_RawData *sample);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static void UART_SendString(const char *text)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)strlen(text), 200U);
}

static void FormatFixed100(char *text, size_t text_size, int32_t value_x100)
{
  const char *sign = "";
  uint32_t magnitude;

  if (value_x100 < 0)
  {
    sign = "-";
    magnitude = (uint32_t)(-value_x100);
  }
  else
  {
    magnitude = (uint32_t)value_x100;
  }

  (void)snprintf(text, text_size, "%s%lu.%02lu", sign,
                 (unsigned long)(magnitude / 100U),
                 (unsigned long)(magnitude % 100U));
}

static HAL_StatusTypeDef MPU6050_WriteRegister(uint8_t reg, uint8_t value)
{
  return HAL_I2C_Mem_Write(&hi2c1, mpu6050_address, reg,
                           I2C_MEMADD_SIZE_8BIT, &value, 1U, 100U);
}

static HAL_StatusTypeDef MPU6050_ReadRegisters(uint8_t reg, uint8_t *data,
                                               uint16_t length)
{
  return HAL_I2C_Mem_Read(&hi2c1, mpu6050_address, reg,
                          I2C_MEMADD_SIZE_8BIT, data, length, 100U);
}

static HAL_StatusTypeDef MPU6050_Init(void)
{
  uint8_t who_am_i = 0U;

  if (HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR_LOW, 3U, 100U) == HAL_OK)
  {
    mpu6050_address = MPU6050_ADDR_LOW;
  }
  else if (HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR_HIGH, 3U, 100U) == HAL_OK)
  {
    mpu6050_address = MPU6050_ADDR_HIGH;
  }
  else
  {
    return HAL_ERROR;
  }

  if (MPU6050_ReadRegisters(MPU6050_REG_WHO_AM_I, &who_am_i, 1U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (who_am_i != MPU6050_WHO_AM_I_VALUE)
  {
    return HAL_ERROR;
  }

  /* Reset, select PLL clock, 100 Hz sample rate and enable the low-pass filter. */
  if (MPU6050_WriteRegister(MPU6050_REG_PWR_MGMT_1, 0x80U) != HAL_OK)
  {
    return HAL_ERROR;
  }
  HAL_Delay(100U);

  if ((MPU6050_WriteRegister(MPU6050_REG_PWR_MGMT_1, 0x01U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_SMPLRT_DIV, 9U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_CONFIG, 0x03U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_GYRO_CFG, 0x00U) != HAL_OK) ||
      (MPU6050_WriteRegister(MPU6050_REG_ACCEL_CFG, 0x00U) != HAL_OK))
  {
    return HAL_ERROR;
  }

  HAL_Delay(50U);
  return HAL_OK;
}

static HAL_StatusTypeDef MPU6050_ReadRaw(MPU6050_RawData *sample)
{
  uint8_t data[14];

  if (MPU6050_ReadRegisters(MPU6050_REG_ACCEL_XOUT, data, sizeof(data)) != HAL_OK)
  {
    return HAL_ERROR;
  }

  sample->ax_raw = (int16_t)((uint16_t)data[0] << 8U | data[1]);
  sample->ay_raw = (int16_t)((uint16_t)data[2] << 8U | data[3]);
  sample->az_raw = (int16_t)((uint16_t)data[4] << 8U | data[5]);
  sample->temp_raw = (int16_t)((uint16_t)data[6] << 8U | data[7]);
  sample->gx_raw = (int16_t)((uint16_t)data[8] << 8U | data[9]);
  sample->gy_raw = (int16_t)((uint16_t)data[10] << 8U | data[11]);
  sample->gz_raw = (int16_t)((uint16_t)data[12] << 8U | data[13]);

  return HAL_OK;
}

static void MPU6050_CalibrateGyroscope(void)
{
  MPU6050_RawData sample;
  int64_t sum_x = 0;
  int64_t sum_y = 0;
  int64_t sum_z = 0;
  uint32_t valid_samples = 0U;

  UART_SendString("Keep the board still: calibrating gyro...\r\n");

  for (uint32_t i = 0U; i < 300U; i++)
  {
    if (MPU6050_ReadRaw(&sample) == HAL_OK)
    {
      sum_x += sample.gx_raw;
      sum_y += sample.gy_raw;
      sum_z += sample.gz_raw;
      valid_samples++;
    }
    HAL_Delay(3U);
  }

  if (valid_samples > 0U)
  {
    gyro_x_bias = (int32_t)(sum_x / (int64_t)valid_samples);
    gyro_y_bias = (int32_t)(sum_y / (int64_t)valid_samples);
    gyro_z_bias = (int32_t)(sum_z / (int64_t)valid_samples);
  }

  UART_SendString("Calibration complete. Move or tilt the board.\r\n\r\n");
}

static void MPU6050_UpdateAngles(const MPU6050_RawData *sample, float dt_s)
{
  const float ax_g = (float)sample->ax_raw / 16384.0f;
  const float ay_g = (float)sample->ay_raw / 16384.0f;
  const float az_g = (float)sample->az_raw / 16384.0f;
  const float gx_dps = (float)(sample->gx_raw - gyro_x_bias) / 131.0f;
  const float gy_dps = (float)(sample->gy_raw - gyro_y_bias) / 131.0f;
  const float gz_dps = (float)(sample->gz_raw - gyro_z_bias) / 131.0f;
  const float roll_from_acc = atan2f(ay_g, az_g) * RAD_TO_DEG;
  const float pitch_from_acc = atan2f(-ax_g,
                                     sqrtf(ay_g * ay_g + az_g * az_g)) *
                               RAD_TO_DEG;

  if (angle_initialized == 0U)
  {
    roll_deg = roll_from_acc;
    pitch_deg = pitch_from_acc;
    yaw_deg = 0.0f;
    angle_initialized = 1U;
    return;
  }

  /* Complementary filter: gyro is fast, accelerometer corrects long-term drift. */
  roll_deg = 0.98f * (roll_deg + gx_dps * dt_s) + 0.02f * roll_from_acc;
  pitch_deg = 0.98f * (pitch_deg + gy_dps * dt_s) + 0.02f * pitch_from_acc;
  yaw_deg += gz_dps * dt_s;
}

static void MPU6050_PrintSample(const MPU6050_RawData *sample)
{
  char line[320];
  char gx_text[16];
  char gy_text[16];
  char gz_text[16];
  char roll_text[16];
  char pitch_text[16];
  char yaw_text[16];
  char temp_text[16];
  const int32_t ax_mg = ((int32_t)sample->ax_raw * 1000) / 16384;
  const int32_t ay_mg = ((int32_t)sample->ay_raw * 1000) / 16384;
  const int32_t az_mg = ((int32_t)sample->az_raw * 1000) / 16384;
  const int32_t gx_x100 = ((int32_t)sample->gx_raw - gyro_x_bias) * 100 / 131;
  const int32_t gy_x100 = ((int32_t)sample->gy_raw - gyro_y_bias) * 100 / 131;
  const int32_t gz_x100 = ((int32_t)sample->gz_raw - gyro_z_bias) * 100 / 131;
  const int32_t temp_x100 = ((int32_t)sample->temp_raw * 100) / 340 + 3653;

  FormatFixed100(gx_text, sizeof(gx_text), gx_x100);
  FormatFixed100(gy_text, sizeof(gy_text), gy_x100);
  FormatFixed100(gz_text, sizeof(gz_text), gz_x100);
  FormatFixed100(roll_text, sizeof(roll_text), (int32_t)(roll_deg * 100.0f));
  FormatFixed100(pitch_text, sizeof(pitch_text), (int32_t)(pitch_deg * 100.0f));
  FormatFixed100(yaw_text, sizeof(yaw_text), (int32_t)(yaw_deg * 100.0f));
  FormatFixed100(temp_text, sizeof(temp_text), temp_x100);

  (void)snprintf(line, sizeof(line),
                 "\r\n"
                 "ACC[mg] X=%ld Y=%ld Z=%ld\r\n"
                 "GYRO[dps] X=%s Y=%s Z=%s\r\n"
                 "ANGLE[deg] Roll=%s Pitch=%s Yaw=%s(relative/drifts)\r\n"
                 "TEMP=%s C\r\n"
                 "----------------------------------------\r\n\r\n",
                 (long)ax_mg, (long)ay_mg, (long)az_mg,
                 gx_text, gy_text, gz_text,
                 roll_text, pitch_text, yaw_text, temp_text);
  UART_SendString(line);
}
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */
  MPU6050_RawData sample;
  uint32_t last_sample_tick;
  uint32_t last_print_tick;
  uint8_t sensor_ready = 0U;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  UART_SendString("\r\nSTM32F407 I2C1 + onboard MPU6050 test.\r\n");
  UART_SendString("I2C1: PB8=SCL, PB9=SDA, UART1: 115200 8-N-1.\r\n");

  if (MPU6050_Init() == HAL_OK)
  {
    char info[80];
    (void)snprintf(info, sizeof(info),
                   "MPU6050 found: address=0x%02X, WHO_AM_I=0x68.\r\n",
                   (unsigned int)(mpu6050_address >> 1U));
    UART_SendString(info);
    MPU6050_CalibrateGyroscope();
    sensor_ready = 1U;
  }
  else
  {
    UART_SendString("ERROR: MPU6050 not found at 0x68 or 0x69.\r\n");
    UART_SendString("Check that the complete Batianhu V2 baseboard is connected.\r\n");
  }

  last_sample_tick = HAL_GetTick();
  last_print_tick = last_sample_tick;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    const uint32_t now = HAL_GetTick();

    if ((sensor_ready != 0U) && ((now - last_sample_tick) >= 10U))
    {
      const float dt_s = (float)(now - last_sample_tick) / 1000.0f;
      last_sample_tick = now;

      if (MPU6050_ReadRaw(&sample) == HAL_OK)
      {
        MPU6050_UpdateAngles(&sample, dt_s);

        /*
         * 姿态滤波仍然每10 ms更新，串口每2秒显示一次，
         * 兼顾角度计算精度和阅读体验。
         */
        if ((now - last_print_tick) >= 2000U)
        {
          last_print_tick = now;
          MPU6050_PrintSample(&sample);
        }
      }
      else
      {
        UART_SendString("I2C read failed; retrying sensor detection.\r\n");
        sensor_ready = 0U;
      }
    }

    if ((sensor_ready == 0U) && ((now - last_print_tick) >= 1000U))
    {
      last_print_tick = now;
      if (MPU6050_Init() == HAL_OK)
      {
        UART_SendString("MPU6050 connected again.\r\n");
        angle_initialized = 0U;
        MPU6050_CalibrateGyroscope();
        sensor_ready = 1U;
        last_sample_tick = HAL_GetTick();
      }
      else
      {
        UART_SendString("Waiting for MPU6050 at I2C address 0x68/0x69...\r\n");
      }
    }
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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

static void MX_I2C1_Init(void)
{
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
  __HAL_RCC_GPIOB_CLK_ENABLE();
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
