#ifndef LED_H
#define LED_H

#include <rtthread.h>
#include "main.h"

#define LED_SW_PWM_PERIOD_MS 10
#define LED_BREATHE_LEVELS 100
#define LED_PWM_MIN_DUTY 4
#define LED_BLINK_INTERVAL 500
#define LED_BREATHE_PERIOD 2500

#define LED_ON() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)
#define LED_OFF() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)

enum led_mode
{
    LED_MODE_GPIO = 0,
    LED_MODE_PWM = 1,
};

void led_entry(void *parameter);
void led_on(void);
void led_off(void);
void led_toggle(void);
void led_set_mode(enum led_mode mode);
enum led_mode led_get_mode(void);
void led_set_brightness(rt_uint8_t brightness);
void led_set_blink_interval(rt_uint32_t interval_ms);
void led_set_breathe_period(rt_uint32_t period_ms);

#endif
