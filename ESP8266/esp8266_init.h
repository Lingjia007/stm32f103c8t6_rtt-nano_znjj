#ifndef ESP8266_INIT_H
#define ESP8266_INIT_H

#include "platform_wifi_esp8266_impl.h"
#include "platform_mqtt_esp8266_impl.h"
#include "platform_uart_stm32_impl.h"

extern uart_stm32_t g_usart1_esp8266;
extern wifi_esp8266_t g_esp8266_wifi;
extern mqtt_esp8266_t g_esp8266_mqtt;

void esp8266_platform_init(void);
void esp8266_uart_enable_it(void);

#endif
