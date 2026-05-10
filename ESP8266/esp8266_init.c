#include "esp8266_init.h"
#include "esp8266_config.h"
#include "usart.h"

uart_stm32_t g_usart1_esp8266;
wifi_esp8266_t g_esp8266_wifi;
mqtt_esp8266_t g_esp8266_mqtt;

void esp8266_platform_init(void)
{
    platform_uart_stm32_register(&g_usart1_esp8266, &huart1, "usart1_esp8266");
    platform_wifi_esp8266_register(&g_esp8266_wifi, &g_usart1_esp8266.base, "esp8266_wifi");
    platform_mqtt_esp8266_register(&g_esp8266_mqtt, &g_esp8266_wifi, "esp8266_mqtt");
}

void esp8266_uart_enable_it(void)
{
    UART_ENABLE_IT(&g_usart1_esp8266.base, PLATFORM_UART_IT_RXNE);
    UART_ENABLE_IT(&g_usart1_esp8266.base, PLATFORM_UART_IT_IDLE);
}
