#ifndef KEY_H
#define KEY_H

#include "main.h"
#include "rtthread.h"

#define KEY_COUNT 4

#define KEY_STATE_RELEASED 0
#define KEY_STATE_PRESSED 1

#define KEY_EVENT_NONE 0x00
#define KEY_EVENT_PRESS 0x01
#define KEY_EVENT_RELEASE 0x02
#define KEY_EVENT_LONG_PRESS 0x04
#define KEY_EVENT_CLICK 0x08
#define KEY_EVENT_DOUBLE_CLICK 0x10

#define KEY_DEFAULT_DEBOUNCE_TIME 20
#define KEY_DEFAULT_LONG_PRESS_TIME 1500
#define KEY_DEFAULT_CLICK_TIME 300

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t active_low;
    uint8_t last_state;
    uint8_t stable_state;
    uint32_t last_tick;
    uint32_t press_tick;
    uint32_t debounce_time;
    uint32_t long_press_time;
    uint32_t click_time;
    uint8_t click_count;
    uint32_t last_release_tick;
} key_t;

typedef void (*key_callback_t)(uint8_t key_id, uint8_t event, void *user_data);

void key_init(void);
void key_register_callback(key_callback_t callback, void *user_data);
uint8_t key_get_state(uint8_t key_id);
void key_scan(void);
void key_thread_entry(void *parameter);

#endif
