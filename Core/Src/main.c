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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rtthread.h"
#include "oled.h"
#include "bmp.h"
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
uint8_t t = ' ';

static rt_thread_t led_thread = RT_NULL;
static rt_thread_t oled_thread = RT_NULL;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void led_thread_entry(void *parameter)
{
  while (1)
  {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    rt_thread_mdelay(500);
  }
}

static void oled_thread_entry(void *parameter)
{
  while (1)
  {
    OLED_ShowPicture(0, 0, 128, 64, BMP1, 1);
    OLED_Refresh();
    rt_thread_mdelay(500);

    OLED_Clear();
    OLED_ShowChinese(0, 0, 0, 16, 1);
    OLED_ShowChinese(18, 0, 1, 16, 1);
    OLED_ShowChinese(36, 0, 2, 16, 1);
    OLED_ShowChinese(54, 0, 3, 16, 1);
    OLED_ShowChinese(72, 0, 4, 16, 1);
    OLED_ShowChinese(90, 0, 5, 16, 1);
    OLED_ShowChinese(108, 0, 6, 16, 1);
    OLED_ShowString(8, 16, (uint8_t *)"ZHONGJINGYUAN", 16, 1);
    OLED_ShowString(20, 32, (uint8_t *)"2014/05/01", 16, 1);
    OLED_ShowString(0, 48, (uint8_t *)"ASCII:", 16, 1);
    OLED_ShowString(63, 48, (uint8_t *)"CODE:", 16, 1);
    OLED_ShowChar(48, 48, t, 16, 1);
    t++;
    if (t > '~')
      t = ' ';
    OLED_ShowNum(103, 48, t, 3, 16, 1);
    OLED_Refresh();
    rt_thread_mdelay(500);

    OLED_Clear();
    OLED_ShowChinese(0, 0, 0, 16, 1);
    OLED_ShowChinese(16, 0, 0, 24, 1);
    OLED_ShowChinese(24, 20, 0, 32, 1);
    OLED_ShowChinese(64, 0, 0, 64, 1);
    OLED_Refresh();
    rt_thread_mdelay(500);

    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t *)"ABC", 8, 1);
    OLED_ShowString(0, 8, (uint8_t *)"ABC", 12, 1);
    OLED_ShowString(0, 20, (uint8_t *)"ABC", 16, 1);
    OLED_ShowString(0, 36, (uint8_t *)"ABC", 24, 1);
    OLED_Refresh();
    rt_thread_mdelay(500);
    OLED_ScrollDisplay(11, 4, 1);
  }
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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear();
  OLED_DisplayTurn(0);
  OLED_ColorTurn(0);
  OLED_ShowString(0, 0, (uint8_t *)"Hello RTThread!", 16, 1);
  OLED_Refresh();

  led_thread = rt_thread_create("led",
                                led_thread_entry,
                                RT_NULL,
                                256,
                                20,
                                20);
  if (led_thread != RT_NULL)
    rt_thread_startup(led_thread);

  oled_thread = rt_thread_create("oled",
                                 oled_thread_entry,
                                 RT_NULL,
                                 1024,
                                 21,
                                 20);
  if (oled_thread != RT_NULL)
    rt_thread_startup(oled_thread);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    rt_thread_mdelay(1000);
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

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM4 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
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
