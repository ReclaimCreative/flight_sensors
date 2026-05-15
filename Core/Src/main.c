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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "vl53l7cx_api.h"
#include <stdio.h>
#include <math.h>

__attribute__((naked)) void _printf_float_support(void) {}
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

/* USER CODE BEGIN PV */
VL53L7CX_Configuration sensor_a;
VL53L7CX_Configuration sensor_b;
VL53L7CX_ResultsData   results_a;
VL53L7CX_ResultsData   results_b;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void I2C_Scan(I2C_HandleTypeDef *hi2c, const char *label) {
    printf("Scanning %s...\r\n", label);
    int found = 0;
    for (uint16_t addr = 1; addr < 128; addr++) {
        HAL_StatusTypeDef result = HAL_I2C_IsDeviceReady(hi2c, addr << 1, 5, 100);
        if (result == HAL_OK) {
            printf("Found at 0x%02X (8-bit: 0x%02X)\r\n", (unsigned)addr, (unsigned)(addr << 1));
            found++;
        }
    }
    if (!found) printf("No devices found on %s\r\n", label);
    printf("Scan complete.\r\n");
}
void I2C_Recover(void) {

    // ---- I2C2 recovery (sensor A — PB10=SCL, PB11=SDA) ----
    HAL_I2C_DeInit(&hi2c2);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Hold sensor A in reset so it doesn't fight the bus
    HAL_GPIO_WritePin(I2C_RST_L_GPIO_Port, I2C_RST_L_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Release both lines high
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(10);

    // Clock out 9 pulses to release any stuck slave
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    // Generate STOP condition — SDA low then high while SCL high
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(10);

    MX_I2C2_Init();
    HAL_Delay(100);

    // ---- I2C1 recovery (sensor B — PB6=SCL, PB7=SDA) ----
    HAL_I2C_DeInit(&hi2c1);

    // Hold sensor B in reset
    HAL_GPIO_WritePin(I2C_RST_R_GPIO_Port, I2C_RST_R_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(10);

    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    // Generate STOP condition
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(10);

    MX_I2C1_Init();
    HAL_Delay(100);

    // Release both sensors from reset after bus is clean
    HAL_GPIO_WritePin(I2C_RST_L_GPIO_Port, I2C_RST_L_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_RST_R_GPIO_Port, I2C_RST_R_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}
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

  MX_USART2_UART_Init();


  MX_USART3_UART_Init();

   /* USER CODE BEGIN 2 */
// Bring sensor B up
HAL_GPIO_WritePin(PWREN_R_GPIO_Port,   PWREN_R_Pin,   GPIO_PIN_SET);
HAL_GPIO_WritePin(I2C_RST_R_GPIO_Port, I2C_RST_R_Pin, GPIO_PIN_SET);  // hold in reset
HAL_GPIO_WritePin(LPn_R_GPIO_Port,     LPn_R_Pin,     GPIO_PIN_RESET);
HAL_Delay(100);
HAL_GPIO_WritePin(I2C_RST_R_GPIO_Port, I2C_RST_R_Pin, GPIO_PIN_RESET); // release reset
HAL_GPIO_WritePin(LPn_R_GPIO_Port,     LPn_R_Pin,     GPIO_PIN_SET);
HAL_Delay(2000);


HAL_GPIO_WritePin(PWREN_L_GPIO_Port,   PWREN_L_Pin,   GPIO_PIN_SET);
HAL_GPIO_WritePin(I2C_RST_L_GPIO_Port, I2C_RST_L_Pin, GPIO_PIN_SET);  // hold in reset
HAL_GPIO_WritePin(LPn_L_GPIO_Port,     LPn_L_Pin,     GPIO_PIN_RESET);
HAL_Delay(100);
HAL_GPIO_WritePin(I2C_RST_L_GPIO_Port, I2C_RST_L_Pin, GPIO_PIN_RESET); // release reset
HAL_GPIO_WritePin(LPn_L_GPIO_Port,     LPn_L_Pin,     GPIO_PIN_SET);
HAL_Delay(2000);
printf("Pins set");
printf("\r\n");

MX_I2C1_Init();
MX_I2C2_Init();
I2C_Scan(&hi2c1, "I2C1");
I2C_Scan(&hi2c2, "I2C2"); 

 while(1) {}
 
// Sensor A only
sensor_a.platform.address  = 0x52;
sensor_a.platform.hi2c     = &hi2c1;
sensor_a.platform.lpn_port = LPn_L_GPIO_Port;
sensor_a.platform.lpn_pin  = LPn_L_Pin;

uint8_t is_alive = 0;
vl53l7cx_is_alive(&sensor_a, &is_alive);
printf("alive=%d\r\n", is_alive);

printf("Init A...\r\n");
uint8_t status = vl53l7cx_init(&sensor_a);
printf("status=%d\r\n", status);

if (status == VL53L7CX_STATUS_OK) {

    vl53l7cx_set_resolution(&sensor_a, VL53L7CX_RESOLUTION_4X4);
    vl53l7cx_set_ranging_frequency_hz(&sensor_a, 10);
    vl53l7cx_start_ranging(&sensor_a);
    printf("A ranging\r\n");
}





  MX_I2C2_Init();
sensor_b.platform.address  = 0x52;
sensor_b.platform.hi2c     = &hi2c2;
sensor_b.platform.lpn_port = LPn_R_GPIO_Port;
sensor_b.platform.lpn_pin  = LPn_R_Pin;

uint8_t is_alive_b = 0;
status = vl53l7cx_is_alive(&sensor_b, &is_alive_b);
printf("Sensor B alive: status=%d alive=%d\r\n", status, is_alive_b);

printf("Initializing sensor B...\r\n");
status = vl53l7cx_init(&sensor_b);
printf("Sensor B init status: %d\r\n", status);

if (status == VL53L7CX_STATUS_OK) {
    printf("Sensor B OK\r\n");
    vl53l7cx_set_resolution(&sensor_b, VL53L7CX_RESOLUTION_4X4);
    vl53l7cx_set_ranging_frequency_hz(&sensor_b, 10);
    vl53l7cx_start_ranging(&sensor_b);
} else {
    printf("Sensor B failed!\r\n");
}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  float d = 240.0f;
  float two_d = 2.0f * d;
  float d_squared = d * d;
  float x, y;
  float cos_A, sin_A;
  uint8_t buf[10];  
  while (1)
{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


uint8_t ready_a = 0;
uint8_t ready_b = 0;
while (!ready_a) {
    vl53l7cx_check_data_ready(&sensor_a, &ready_a);
    HAL_Delay(5);
}
vl53l7cx_get_ranging_data(&sensor_a, &results_a);
// printf("A");
// for (int i = 0; i < 16; i++) printf(",%d", (int)results_a.distance_mm[i]);
// for (int i = 0; i < 16; i++) printf(",%u", (unsigned)results_a.target_status[i]);
// printf("\r\n");
int a_target = 0;
int cnt = 0;
int valid_targets[16];
for (int i = 8; i < 11; i++) {
      if (results_a.distance_mm[i] > 80 && results_a.distance_mm[i] <= 500) {
          
            valid_targets[cnt++] = results_a.distance_mm[i];


      }
    }
int total = 0;
for (int i = 0; i < cnt; i++) total += valid_targets[i];
a_target = (cnt > 0) ? total / cnt : 0;
// printf("A target: %d\r\n", a_target);

while (!ready_b) {
    vl53l7cx_check_data_ready(&sensor_b, &ready_b);
    HAL_Delay(5);
}
vl53l7cx_get_ranging_data(&sensor_b, &results_b);
// printf("B");
// for (int i = 0; i < 16; i++) printf(",%d", (int)results_b.distance_mm[i]);
// for (int i = 0; i < 16; i++) printf(",%u", (unsigned)results_b.target_status[i]);
// printf("\r\n");
int b_target = 0;
cnt = 0;
for (int i = 8; i < 11; i++) {
      if (results_b.distance_mm[i] > 80 && results_b.distance_mm[i] <= 500) {
          
            valid_targets[cnt++] = results_b.distance_mm[i];
      }
    }
total = 0;
for (int i = 0; i < cnt; i++) total += valid_targets[i];
b_target = (cnt > 0) ? total / cnt : 0;
// printf("B target: %d\r\n", b_target);
// // ADD THIS:
// printf("debug: a=%d b=%d\r\n", a_target, b_target);

cos_A = (float)(d_squared + a_target*a_target - b_target*b_target) / (float)(two_d * a_target);
// printf("debug cosA=%.4f sinA=%.4f\r\n", cos_A, sin_A);

sin_A = sqrtf(1.0f - cos_A * cos_A);


x = a_target * cos_A;
y = a_target * sin_A;

//Temporary integer debug to confirm values are correct
int cos_scaled = (int)(cos_A * 10000);
int sin_scaled = (int)(sin_A * 10000);
int x_scaled = (int)x;
int y_scaled = (int)y;
printf("debug: cosA=%d sinA=%d x=%d y=%d\r\n", 
       cos_scaled, sin_scaled, x_scaled, y_scaled);


buf[0] = 0xAA;              // start marker
memcpy(&buf[1], &x, 4);    // 4 bytes of float x
memcpy(&buf[5], &y, 4);    // 4 bytes of float y
buf[9] = 0xFF;              // end marker

HAL_UART_Transmit(&huart3, buf, 10, HAL_MAX_DELAY);

  // uint8_t buf[10];
  // HAL_UART_Receive(&huart1, buf, 10, HAL_MAX_DELAY);

  // if (buf[0] == 0xAA && buf[9] == 0xFF) {
  //     float x, y;
  //     memcpy(&x, &buf[1], 4);
  //     memcpy(&y, &buf[5], 4);
  // }

  /* USER CODE END 3 */
}
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
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
