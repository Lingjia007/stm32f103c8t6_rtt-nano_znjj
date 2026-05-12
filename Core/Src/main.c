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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "rtthread.h"
#include "oled.h"
#include "esp8266_init.h"
#include "dht11.h"
#include "platform_mqtt.h"
#include "esp8266_config.h"

#define DHT11_UPLOAD_ENABLED 1
#define DHT11_UPLOAD_INTERVAL 5000
#define DHT11_READ_RETRY 3
#define MQTT_AUTO_CONNECT 1
#define MQTT_RECONNECT_INTERVAL 30000
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
static rt_thread_t dht11_thread = RT_NULL;
static rt_thread_t mqtt_thread = RT_NULL;

static rt_mutex_t g_esp8266_mutex = RT_NULL;

static uint8_t g_mqtt_configured = 0;

static dht11_data_t g_dht11_data;
static uint8_t g_dht11_valid = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void dwt_init(void);
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
    // OLED_ShowPicture(0, 0, 128, 64, BMP1, 1);
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
    OLED_Refresh();
    rt_thread_mdelay(500);
    OLED_ScrollDisplay(11, 4, 1);
  }
}

static void dht11_thread_entry(void *parameter)
{
  rt_thread_mdelay(3000);
  while (1)
  {
    platform_mqtt_property_t props[3];
    dht11_data_t dht11_data;
    int prop_count = 0;
    int16_t ret;
    int dht11_ok = 0;
    int retry;

    for (retry = 0; retry < DHT11_READ_RETRY; retry++)
    {
      if (dht11_read(&dht11_data) == DHT11_OK)
      {
        dht11_ok = 1;
        break;
      }
      rt_kprintf("  DHT11 read retry %d/%d\n", retry + 1, DHT11_READ_RETRY);
      rt_thread_mdelay(200);
    }

    memset(props, 0, sizeof(props));

    strncpy(props[0].key, "BSP_LED", sizeof(props[0].key) - 1);
    props[0].value_int = 1;
    props[0].value_type = PLATFORM_MQTT_VALUE_BOOL;
    prop_count = 1;

    if (dht11_ok)
    {
      strncpy(props[1].key, "TEMPERATURE", sizeof(props[1].key) - 1);
      props[1].value_int = dht11_data.temperature_int;
      props[1].value_type = PLATFORM_MQTT_VALUE_INT;

      strncpy(props[2].key, "HUMIDITY", sizeof(props[2].key) - 1);
      props[2].value_int = dht11_data.humidity_int;
      props[2].value_type = PLATFORM_MQTT_VALUE_INT;

      prop_count = 3;
      rt_kprintf("  DHT11: Temp=%dC, Humi=%d%%\n",
                 dht11_data.temperature_int, dht11_data.humidity_int);
    }
    else
    {
      rt_kprintf("  DHT11 read failed after %d retries\n", DHT11_READ_RETRY);
    }

    rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
    ret = MQTT_PUBLISH_PROPERTY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                props, prop_count, "001");
    rt_mutex_release(g_esp8266_mutex);

    if (ret == PLATFORM_MQTT_OK)
      rt_kprintf("Publish OK! (%d properties)\n", prop_count);
    else
      rt_kprintf("Publish FAILED! (error=%d)\n", ret);

    rt_thread_mdelay(DHT11_UPLOAD_INTERVAL);
  }
}

static int mqtt_do_connect(void)
{
  int16_t ret;

  if (!g_mqtt_configured)
  {
    platform_mqtt_user_config_t config;

    memset(&config, 0, sizeof(config));
    strncpy(config.client_id, ONENET_DEVICE_NAME, sizeof(config.client_id) - 1);
    strncpy(config.username, ONENET_PRODUCT_ID, sizeof(config.username) - 1);
    strncpy(config.password, ONENET_MQTT_TOKEN, sizeof(config.password) - 1);

    ret = MQTT_USERCFG(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID, &config);
    if (ret != PLATFORM_MQTT_OK)
    {
      rt_kprintf("MQTT config FAILED!\n");
      return -1;
    }
    g_mqtt_configured = 1;
    rt_kprintf("MQTT configured!\n");
  }
  else
  {
    rt_kprintf("MQTT already configured, reconnecting...\n");
  }

  ret = MQTT_CONNECT(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                     ONENET_MQTT_HOST, ONENET_MQTT_PORT, 1);
  if (ret != PLATFORM_MQTT_OK)
  {
    rt_kprintf("MQTT connect FAILED! resetting config flag\n");
    g_mqtt_configured = 0;
    return -1;
  }

  rt_kprintf("MQTT connected!\n");
  return 0;
}

static void mqtt_do_subscribe(void)
{
  MQTT_SUBSCRIBE(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                 ONENET_TOPIC_PROPERTY_POST_REPLY, 1);
  MQTT_SUBSCRIBE(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                 ONENET_TOPIC_PROPERTY_SET, 1);
  MQTT_SUBSCRIBE(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                 ONENET_TOPIC_PROPERTY_GET, 1);
  MQTT_SUBSCRIBE(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                 ONENET_TOPIC_OTA_INFORM, 1);
  rt_kprintf("MQTT subscribed!\n");
}

static void mqtt_thread_entry(void *parameter)
{
  rt_thread_mdelay(200);

  while (1)
  {
    rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);

    if (MQTT_CHECK_CONNECTED(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID) != PLATFORM_MQTT_OK)
    {
      rt_kprintf("MQTT not connected, checking WiFi...\n");

      if (WIFI_AT_TEST(&g_esp8266_wifi.base) != PLATFORM_WIFI_OK)
      {
        rt_kprintf("ESP8266 AT test FAILED! Retrying...\n");
        rt_mutex_release(g_esp8266_mutex);
        rt_thread_mdelay(MQTT_RECONNECT_INTERVAL);
        continue;
      }

      char ip_buf[64];
      if (WIFI_GET_IP(&g_esp8266_wifi.base, ip_buf, sizeof(ip_buf)) != PLATFORM_WIFI_OK)
      {
        rt_kprintf("WiFi not connected, reconnecting...\n");
        int16_t wifi_ret = WIFI_JOIN_AP(&g_esp8266_wifi.base,
                                        ESP8266_WIFI_SSID,
                                        ESP8266_WIFI_PASSWORD);
        if (wifi_ret != PLATFORM_WIFI_OK)
        {
          rt_kprintf("WiFi reconnect FAILED! (error=%d)\n", wifi_ret);
          rt_mutex_release(g_esp8266_mutex);
          rt_thread_mdelay(MQTT_RECONNECT_INTERVAL);
          continue;
        }
        rt_kprintf("WiFi reconnected!\n");
        rt_thread_mdelay(1000);
      }
      else
      {
        rt_kprintf("WiFi connected (IP: %s)\n", ip_buf);
      }

      rt_kprintf("Trying MQTT connect...\n");
      if (mqtt_do_connect() == 0)
      {
        mqtt_do_subscribe();
      }
    }

    rt_mutex_release(g_esp8266_mutex);

    rt_thread_mdelay(MQTT_RECONNECT_INTERVAL);
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  dwt_init();
  OLED_Init();
  OLED_Clear();
  OLED_DisplayTurn(0);
  OLED_ColorTurn(0);
  OLED_ShowString(0, 0, (uint8_t *)"Hello RTThread!", 16, 1);
  OLED_Refresh();

  esp8266_platform_init();
  esp8266_uart_enable_it();

  g_esp8266_mutex = rt_mutex_create("esp8266", RT_IPC_FLAG_PRIO);

  led_thread = rt_thread_create("led",
                                led_thread_entry,
                                RT_NULL,
                                128,
                                24,
                                10);
  if (led_thread != RT_NULL)
    rt_thread_startup(led_thread);

  oled_thread = rt_thread_create("oled",
                                 oled_thread_entry,
                                 RT_NULL,
                                 256,
                                 24,
                                 10);
  if (oled_thread != RT_NULL)
    rt_thread_startup(oled_thread);

  dht11_thread = rt_thread_create("dht11",
                                  dht11_thread_entry,
                                  RT_NULL,
                                  1024,
                                  20,
                                  25);
  if (dht11_thread != RT_NULL)
    rt_thread_startup(dht11_thread);

#if MQTT_AUTO_CONNECT
  mqtt_thread = rt_thread_create("mqtt",
                                 mqtt_thread_entry,
                                 RT_NULL,
                                 1536,
                                 21,
                                 20);
  if (mqtt_thread != RT_NULL)
    rt_thread_startup(mqtt_thread);
#endif
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

#define DWT_CYCCNT ((volatile uint32_t *)0xE0001004)
#define DWT_CONTROL ((volatile uint32_t *)0xE0001000)
#define DEM_CR ((volatile uint32_t *)0xE000EDFC)

#define DEM_CR_TRCENA (1 << 24)
#define DWT_CR_CYCCNTENA (1 << 0)

static void dwt_init(void)
{
  *DEM_CR |= DEM_CR_TRCENA;
  *DWT_CYCCNT = 0;
  *DWT_CONTROL |= DWT_CR_CYCCNTENA;
}

void rt_hw_us_delay(rt_uint32_t us)
{
  rt_uint32_t start = *DWT_CYCCNT;
  rt_uint32_t cycles = us * (SystemCoreClock / 1000000);

  while ((*DWT_CYCCNT - start) < cycles)
  {
  }
}

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
