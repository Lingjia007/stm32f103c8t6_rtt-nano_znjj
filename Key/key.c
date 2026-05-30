#include "key.h"

static key_t g_keys[KEY_COUNT];
static key_callback_t g_key_callback = RT_NULL;
static void *g_callback_user_data = RT_NULL;

static const struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} key_gpio_map[KEY_COUNT] = {
    {KEY1_GPIO_Port, KEY1_Pin},
    {KEY2_GPIO_Port, KEY2_Pin},
    {KEY3_GPIO_Port, KEY3_Pin},
    {KEY4_GPIO_Port, KEY4_Pin},
};

void key_init(void)
{
    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
        g_keys[i].port = key_gpio_map[i].port;
        g_keys[i].pin = key_gpio_map[i].pin;
        g_keys[i].active_low = 1;
        g_keys[i].last_state = 1;
        g_keys[i].stable_state = KEY_STATE_RELEASED;
        g_keys[i].last_tick = 0;
        g_keys[i].press_tick = 0;
        g_keys[i].debounce_time = KEY_DEFAULT_DEBOUNCE_TIME;
        g_keys[i].long_press_time = KEY_DEFAULT_LONG_PRESS_TIME;
        g_keys[i].click_time = KEY_DEFAULT_CLICK_TIME;
        g_keys[i].click_count = 0;
        g_keys[i].last_release_tick = 0;
    }
}

void key_register_callback(key_callback_t callback, void *user_data)
{
    g_key_callback = callback;
    g_callback_user_data = user_data;
}

uint8_t key_get_state(uint8_t key_id)
{
    if (key_id >= KEY_COUNT)
        return KEY_STATE_RELEASED;
    return g_keys[key_id].stable_state;
}

static uint8_t key_read_raw(uint8_t key_id)
{
    if (key_id >= KEY_COUNT)
        return 1;

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(g_keys[key_id].port, g_keys[key_id].pin);

    if (g_keys[key_id].active_low)
        return (pin_state == GPIO_PIN_RESET) ? 0 : 1;
    else
        return (pin_state == GPIO_PIN_SET) ? 1 : 0;
}

static void key_notify_event(uint8_t key_id, uint8_t event)
{
    if (g_key_callback != RT_NULL)
    {
        g_key_callback(key_id, event, g_callback_user_data);
    }
}

void key_scan(void)
{
    uint32_t current_tick = rt_tick_get();

    for (uint8_t i = 0; i < KEY_COUNT; i++)
    {
        key_t *key = &g_keys[i];
        uint8_t raw_state = key_read_raw(i);

        if (raw_state != key->last_state)
        {
            key->last_state = raw_state;
            key->last_tick = current_tick;
        }

        if ((current_tick - key->last_tick) >= key->debounce_time)
        {
            uint8_t new_stable = raw_state ? KEY_STATE_RELEASED : KEY_STATE_PRESSED;

            if (new_stable != key->stable_state)
            {
                key->stable_state = new_stable;

                if (new_stable == KEY_STATE_PRESSED)
                {
                    key->press_tick = current_tick;
                    key_notify_event(i, KEY_EVENT_PRESS);
                }
                else
                {
                    uint32_t press_duration = current_tick - key->press_tick;

                    if (press_duration >= key->long_press_time)
                    {
                        key_notify_event(i, KEY_EVENT_LONG_PRESS);
                    }
                    else
                    {
                        if ((current_tick - key->last_release_tick) < key->click_time)
                        {
                            key->click_count++;
                            if (key->click_count == 2)
                            {
                                key_notify_event(i, KEY_EVENT_DOUBLE_CLICK);
                                key->click_count = 0;
                            }
                        }
                        else
                        {
                            key->click_count = 1;
                        }
                    }

                    key->last_release_tick = current_tick;
                    key_notify_event(i, KEY_EVENT_RELEASE);

                    if (key->click_count == 1 && press_duration < key->long_press_time)
                    {
                        key_notify_event(i, KEY_EVENT_CLICK);
                        key->click_count = 0;
                    }
                }
            }
        }

        if (key->stable_state == KEY_STATE_PRESSED)
        {
            uint32_t press_duration = current_tick - key->press_tick;
            if (press_duration >= key->long_press_time && press_duration < key->long_press_time + 10)
            {
                key_notify_event(i, KEY_EVENT_LONG_PRESS);
            }
        }
    }
}

void key_thread_entry(void *parameter)
{
    (void)parameter;

    key_init();
    rt_kprintf("[KEY] Key thread started, %d keys initialized\n", KEY_COUNT);

    while (1)
    {
        key_scan();
        rt_thread_mdelay(10);
    }
}
