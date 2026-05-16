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
#include "adc.h"
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
#include "light_sensor.h"
#include "pir_sensor.h"
#include "onenet_kv.h"
#include "onenet_cmd.h"

#define DHT11_UPLOAD_ENABLED 1
#define DHT11_UPLOAD_INTERVAL 3600
#define DHT11_READ_RETRY 3
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
static rt_thread_t light_sensor_thread = RT_NULL;
static rt_thread_t pir_sensor_thread = RT_NULL;
static rt_thread_t mqtt_recv_thread = RT_NULL;
static rt_thread_t mqtt_reply_thread = RT_NULL;

static rt_mutex_t g_esp8266_mutex = RT_NULL;
static rt_sem_t g_mqtt_frame_sem = RT_NULL;
static rt_mq_t g_mqtt_reply_mq = RT_NULL;

typedef struct
{
  char msg_id[32];
} mqtt_reply_msg_t;

static uint8_t g_mqtt_configured = 0;
static uint8_t g_mqtt_connected = 0;

static dht11_data_t g_dht11_data;
static uint8_t g_dht11_valid = 0;

onenet_kv_table_t g_kv_table;
onenet_cmd_ctx_t g_cmd_ctx;

static int g_kv_temperature = 0;
static int g_kv_humidity = 0;
static int g_kv_light = 0;
static uint8_t g_kv_bsp_led = 1;
static char g_kv_pir[ONENET_KV_MAX_STRING_LEN] = "not_detected";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void dwt_init(void);
static int mqtt_reconnect(void);
static void onenet_kv_table_setup(void);
static void bsp_led_on_change(const char *key, void *value, uint8_t value_type);
static void mqtt_frame_isr_cb(void *arg);
static void mqtt_recv_thread_entry(void *parameter);
static void mqtt_reply_thread_entry(void *parameter);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void bsp_led_on_change(const char *key, void *value, uint8_t value_type)
{
  uint8_t led_val = *((uint8_t *)value);
  rt_kprintf("BSP_LED changed -> %d (%s)\n", led_val, led_val ? "blink" : "off");
}

static void onenet_kv_table_setup(void)
{
  onenet_kv_init(&g_kv_table);

  onenet_kv_register(&g_kv_table, "TEMPERATURE",
                     PLATFORM_MQTT_VALUE_INT, &g_kv_temperature, NULL);
  onenet_kv_register(&g_kv_table, "HUMIDITY",
                     PLATFORM_MQTT_VALUE_INT, &g_kv_humidity, NULL);
  onenet_kv_register(&g_kv_table, "LIGHT",
                     PLATFORM_MQTT_VALUE_INT, &g_kv_light, NULL);
  onenet_kv_register(&g_kv_table, "BSP_LED",
                     PLATFORM_MQTT_VALUE_BOOL, &g_kv_bsp_led, bsp_led_on_change);
  onenet_kv_register(&g_kv_table, "PIR",
                     PLATFORM_MQTT_VALUE_STRING, g_kv_pir, NULL);

  onenet_cmd_init(&g_cmd_ctx, &g_kv_table,
                  &g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                  ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
}

static void led_thread_entry(void *parameter)
{
  while (1)
  {
    if (g_kv_bsp_led)
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    else
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
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
    // OLED_ScrollDisplay(11, 4, 1);
  }
}

static void dht11_thread_entry(void *parameter)
{
  rt_thread_mdelay(3000);
  while (1)
  {
    platform_mqtt_property_t props[2];
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

    if (dht11_ok)
    {
      g_kv_temperature = dht11_data.temperature_int;
      g_kv_humidity = dht11_data.humidity_int;

      strncpy(props[0].key, "TEMPERATURE", sizeof(props[0].key) - 1);
      props[0].value_int = g_kv_temperature;
      props[0].value_type = PLATFORM_MQTT_VALUE_INT;

      strncpy(props[1].key, "HUMIDITY", sizeof(props[1].key) - 1);
      props[1].value_int = g_kv_humidity;
      props[1].value_type = PLATFORM_MQTT_VALUE_INT;

      prop_count = 2;
      rt_kprintf("  DHT11: Temp=%dC, Humi=%d%%\n",
                 g_kv_temperature, g_kv_humidity);
    }
    else
    {
      rt_kprintf("  DHT11 read failed after %d retries\n", DHT11_READ_RETRY);
    }

    if (!g_mqtt_connected)
    {
      mqtt_reconnect();
    }

    if (prop_count > 0 && g_mqtt_connected)
    {
      rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
      ret = MQTT_PUBLISH_PROPERTY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                  ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                  props, prop_count, "001");
      rt_mutex_release(g_esp8266_mutex);

      if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("DHT11 Publish OK! (%d properties)\n", prop_count);
      else
        rt_kprintf("DHT11 Publish FAILED! (error=%d)\n", ret);
    }

    rt_thread_mdelay(DHT11_UPLOAD_INTERVAL);
  }
}

static void light_sensor_thread_entry(void *parameter)
{
  uint16_t adc_raw;
  uint8_t light_percentage;
  platform_mqtt_property_t prop;
  int16_t ret;

  rt_thread_mdelay(1000);
  light_sensor_init();

  while (1)
  {
    adc_raw = light_sensor_read_raw();
    light_percentage = light_sensor_read_percentage();

    rt_kprintf("Light Sensor: Raw=%d, Percentage=%d%%\n", adc_raw, light_percentage);

    g_kv_light = light_percentage;

    if (!g_mqtt_connected)
    {
      mqtt_reconnect();
    }

    if (g_mqtt_connected)
    {
      memset(&prop, 0, sizeof(prop));
      strncpy(prop.key, "LIGHT", sizeof(prop.key) - 1);
      prop.value_int = g_kv_light;
      prop.value_type = PLATFORM_MQTT_VALUE_INT;

      rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
      ret = MQTT_PUBLISH_PROPERTY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                  ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                  &prop, 1, "002");
      rt_mutex_release(g_esp8266_mutex);

      if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("Light Publish OK!\n");
      else
        rt_kprintf("Light Publish FAILED! (error=%d)\n", ret);
    }

    rt_thread_mdelay(5000);
  }
}

static void pir_sensor_thread_entry(void *parameter)
{
  uint8_t pir_status;
  platform_mqtt_property_t prop;
  int16_t ret;

  rt_thread_mdelay(1500);
  pir_sensor_init();

  while (1)
  {
    pir_status = pir_sensor_read();

    if (pir_status == PIR_DETECTED)
    {
      rt_kprintf("PIR Sensor: Detected! (Someone is here)\n");
      strncpy(g_kv_pir, "detected", sizeof(g_kv_pir) - 1);
    }
    else
    {
      rt_kprintf("PIR Sensor: Not detected (No one)\n");
      strncpy(g_kv_pir, "not_detected", sizeof(g_kv_pir) - 1);
    }

    if (!g_mqtt_connected)
    {
      mqtt_reconnect();
    }

    if (g_mqtt_connected)
    {
      memset(&prop, 0, sizeof(prop));
      strncpy(prop.key, "PIR", sizeof(prop.key) - 1);
      strncpy(prop.id, g_kv_pir, sizeof(prop.id) - 1);
      prop.value_type = PLATFORM_MQTT_VALUE_STRING;

      rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
      ret = MQTT_PUBLISH_PROPERTY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                  ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                  &prop, 1, "003");
      rt_mutex_release(g_esp8266_mutex);

      if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("PIR Publish OK!\n");
      else
        rt_kprintf("PIR Publish FAILED! (error=%d)\n", ret);
    }

    rt_thread_mdelay(3000);
  }
}

static void mqtt_frame_isr_cb(void *arg)
{
  (void)arg;
  rt_sem_release(g_mqtt_frame_sem);
}

static void mqtt_recv_thread_entry(void *parameter)
{
  static char recv_topic[PLATFORM_MQTT_MAX_TOPIC_LEN];
  static char recv_payload[PLATFORM_MQTT_MAX_PAYLOAD_LEN];
  static char recv_msg_id[32];

  rt_thread_mdelay(4000);
  wifi_esp8266_set_frame_cb(g_esp8266_mqtt.wifi, mqtt_frame_isr_cb, NULL);
  rt_kprintf("[mqtt_recv] ISR-driven listening started\n");

  while (1)
  {
    rt_sem_take(g_mqtt_frame_sem, RT_WAITING_FOREVER);

    if (!g_mqtt_connected)
      continue;

    if (g_esp8266_mqtt.wifi->rx_frame.sta.finsh == 0)
      continue;

    memset(recv_topic, 0, sizeof(recv_topic));
    memset(recv_payload, 0, sizeof(recv_payload));
    memset(recv_msg_id, 0, sizeof(recv_msg_id));

    rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
    int16_t recv_ret = MQTT_CHECK_PROPERTY_SET_RECV(&g_esp8266_mqtt.base,
                                                    recv_topic, recv_payload,
                                                    sizeof(recv_payload), recv_msg_id);
    wifi_esp8266_rx_restart(g_esp8266_mqtt.wifi);
    rt_mutex_release(g_esp8266_mutex);

    if (recv_ret != PLATFORM_MQTT_OK)
      continue;

    if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_POST_REPLY) == 0)
      continue;

    rt_kprintf("[mqtt_recv] topic=%s\n", recv_topic);
    rt_kprintf("[mqtt_recv] payload=%s\n", recv_payload);

    if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_SET) == 0)
    {
      int8_t updated = onenet_kv_parse_set_payload(&g_kv_table, recv_payload);
      rt_kprintf("[mqtt_recv] SET: %d updated, LED=%d, T=%d, H=%d, L=%d\n",
                 updated, g_kv_bsp_led, g_kv_temperature, g_kv_humidity, g_kv_light);

      mqtt_reply_msg_t reply;
      memset(&reply, 0, sizeof(reply));
      strncpy(reply.msg_id, recv_msg_id, sizeof(reply.msg_id) - 1);
      rt_mq_send(g_mqtt_reply_mq, &reply, sizeof(reply));
    }
    else if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_GET) == 0)
    {
      rt_kprintf("[mqtt_recv] GET request\n");
    }
    else if (strcmp(recv_topic, ONENET_TOPIC_OTA_INFORM) == 0)
    {
      rt_kprintf("[mqtt_recv] OTA inform\n");
    }
  }
}

static void mqtt_reply_thread_entry(void *parameter)
{
  mqtt_reply_msg_t reply;

  rt_thread_mdelay(5000);
  rt_kprintf("[mqtt_reply] Reply thread started\n");

  while (1)
  {
    if (rt_mq_recv(g_mqtt_reply_mq, &reply, sizeof(reply), RT_WAITING_FOREVER) != RT_EOK)
      continue;

    if (!g_mqtt_connected)
      continue;

    rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
    int16_t ret = MQTT_PUBLISH_SET_REPLY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                         ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                         reply.msg_id, 200, "user_succ");
    rt_mutex_release(g_esp8266_mutex);

    if (ret == PLATFORM_MQTT_OK)
      rt_kprintf("[mqtt_reply] Reply OK (id=%s)\n", reply.msg_id);
    else
      rt_kprintf("[mqtt_reply] Reply FAILED (id=%s, err=%d)\n", reply.msg_id, ret);
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
      rt_kprintf("MQTT config FAILED! (error=%d)\n", ret);
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
    g_mqtt_connected = 0;
    return -1;
  }

  g_mqtt_connected = 1;
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

static int mqtt_init(void)
{
  int16_t ret;
  char ip_buf[64];

  rt_kprintf("Checking MQTT connection...\n");

  if (MQTT_CHECK_CONNECTED(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID) == PLATFORM_MQTT_OK)
  {
    rt_kprintf("MQTT already connected!\n");
    g_mqtt_connected = 1;
    return 0;
  }

  rt_kprintf("MQTT not connected, checking WiFi...\n");

  if (WIFI_AT_TEST(&g_esp8266_wifi.base) != PLATFORM_WIFI_OK)
  {
    rt_kprintf("ESP8266 AT test FAILED!\n");
    return -1;
  }

  if (WIFI_GET_IP(&g_esp8266_wifi.base, ip_buf, sizeof(ip_buf)) != PLATFORM_WIFI_OK)
  {
    rt_kprintf("WiFi not connected, connecting...\n");
    ret = WIFI_JOIN_AP(&g_esp8266_wifi.base, ESP8266_WIFI_SSID, ESP8266_WIFI_PASSWORD);
    if (ret != PLATFORM_WIFI_OK)
    {
      rt_kprintf("WiFi connect FAILED! (error=%d)\n", ret);
      return -1;
    }
    rt_kprintf("WiFi connected!\n");
    rt_thread_mdelay(1000);
  }
  else
  {
    rt_kprintf("WiFi already connected (IP: %s)\n", ip_buf);
  }

  rt_kprintf("Connecting MQTT...\n");
  if (mqtt_do_connect() != 0)
  {
    return -1;
  }

  mqtt_do_subscribe();
  return 0;
}

static int mqtt_reconnect(void)
{
  int16_t ret;

  if (g_mqtt_connected)
    return 0;

  rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
  ret = mqtt_init();
  rt_mutex_release(g_esp8266_mutex);

  return ret;
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
  MX_ADC1_Init();
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

  onenet_kv_table_setup();

  g_esp8266_mutex = rt_mutex_create("esp8266", RT_IPC_FLAG_PRIO);
  g_mqtt_frame_sem = rt_sem_create("mq_sem", 0, RT_IPC_FLAG_PRIO);
  g_mqtt_reply_mq = rt_mq_create("mq_rep", sizeof(mqtt_reply_msg_t), 4, RT_IPC_FLAG_PRIO);

  rt_mutex_take(g_esp8266_mutex, RT_WAITING_FOREVER);
  if (mqtt_init() == 0)
  {
    rt_kprintf("MQTT init success!\n");
  }
  else
  {
    rt_kprintf("MQTT init failed!\n");
  }
  rt_mutex_release(g_esp8266_mutex);

  led_thread = rt_thread_create("led",
                                led_thread_entry,
                                RT_NULL,
                                96,
                                24,
                                10);
  if (led_thread != RT_NULL)
    rt_thread_startup(led_thread);

  oled_thread = rt_thread_create("oled",
                                 oled_thread_entry,
                                 RT_NULL,
                                 192,
                                 24,
                                 10);
  if (oled_thread != RT_NULL)
    rt_thread_startup(oled_thread);

  dht11_thread = rt_thread_create("dht11",
                                  dht11_thread_entry,
                                  RT_NULL,
                                  768,
                                  20,
                                  10);
  if (dht11_thread != RT_NULL)
    rt_thread_startup(dht11_thread);

  light_sensor_thread = rt_thread_create("light",
                                         light_sensor_thread_entry,
                                         RT_NULL,
                                         640,
                                         22,
                                         10);
  if (light_sensor_thread != RT_NULL)
    rt_thread_startup(light_sensor_thread);

  pir_sensor_thread = rt_thread_create("pir",
                                       pir_sensor_thread_entry,
                                       RT_NULL,
                                       640,
                                       23,
                                       10);
  if (pir_sensor_thread != RT_NULL)
    rt_thread_startup(pir_sensor_thread);

  mqtt_recv_thread = rt_thread_create("mqtt_rcv",
                                      mqtt_recv_thread_entry,
                                      RT_NULL,
                                      384,
                                      15,
                                      20);
  if (mqtt_recv_thread != RT_NULL)
    rt_thread_startup(mqtt_recv_thread);

  mqtt_reply_thread = rt_thread_create("mqtt_rep",
                                       mqtt_reply_thread_entry,
                                       RT_NULL,
                                       576,
                                       18,
                                       20);
  if (mqtt_reply_thread != RT_NULL)
    rt_thread_startup(mqtt_reply_thread);

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
