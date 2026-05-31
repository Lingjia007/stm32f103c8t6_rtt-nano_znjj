#include <rtthread.h>
#include <stdlib.h>
#include "led.h"

static volatile enum led_mode current_mode = LED_MODE_GPIO;
static volatile rt_uint8_t led_running = 1;
static volatile rt_uint8_t current_brightness = 0;
static volatile rt_uint8_t target_brightness = 0;
static volatile rt_uint32_t blink_interval = LED_BLINK_INTERVAL;
static volatile rt_uint32_t breathe_period = LED_BREATHE_PERIOD;

void led_set_mode(enum led_mode mode)
{
    if (mode == current_mode)
        return;

    led_off();
    current_mode = mode;
    led_running = 1;

    if (mode == LED_MODE_PWM)
        target_brightness = 0;
}

enum led_mode led_get_mode(void)
{
    return current_mode;
}

void led_on(void)
{
    led_running = 1;
    if (current_mode == LED_MODE_GPIO)
    {
        LED_ON();
    }
    else
    {
        target_brightness = 0;
    }
}

void led_off(void)
{
    led_running = 0;
    target_brightness = 0;
    current_brightness = 0;
    LED_OFF();
}

void led_toggle(void)
{
    static rt_uint8_t state = 0;
    if (state)
    {
        LED_OFF();
        state = 0;
    }
    else
    {
        LED_ON();
        state = 1;
    }
}

void led_set_brightness(rt_uint8_t brightness)
{
    if (brightness > 100)
        brightness = 100;
    current_mode = LED_MODE_PWM;
    led_running = 1;
    target_brightness = brightness;
}

void led_set_blink_interval(rt_uint32_t interval_ms)
{
    if (interval_ms < 50)
        interval_ms = 50;
    if (interval_ms > 5000)
        interval_ms = 5000;
    blink_interval = interval_ms;
}

void led_set_breathe_period(rt_uint32_t period_ms)
{
    if (period_ms < 500)
        period_ms = 500;
    if (period_ms > 10000)
        period_ms = 10000;
    breathe_period = period_ms;
}

static rt_uint8_t breathe_duty(rt_uint8_t sine_val)
{
    return LED_PWM_MIN_DUTY + (rt_uint8_t)((rt_uint32_t)(100 - LED_PWM_MIN_DUTY) * sine_val / 100);
}

static void led_sw_pwm_output(rt_uint8_t duty)
{
    rt_uint32_t on_time = (LED_SW_PWM_PERIOD_MS * duty) / 100;
    rt_uint32_t off_time = LED_SW_PWM_PERIOD_MS - on_time;

    if (on_time > 0)
    {
        LED_ON();
        rt_thread_mdelay(on_time);
    }
    if (off_time > 0)
    {
        LED_OFF();
        rt_thread_mdelay(off_time);
    }
}

static const rt_uint8_t sine_table[50] = {
    1, 6, 13, 19, 25, 31, 37, 43, 49, 54,
    60, 65, 70, 74, 78, 82, 85, 89, 91, 94,
    96, 97, 99, 100, 100, 100, 100, 99, 97, 96,
    94, 91, 89, 85, 82, 78, 74, 70, 65, 60,
    54, 49, 43, 37, 31, 25, 19, 13, 6, 1};

static rt_uint8_t breathe_lookup(int level)
{
    int idx = level * 49 / (LED_BREATHE_LEVELS - 1);
    if (idx < 0)
        idx = 0;
    if (idx > 49)
        idx = 49;
    return sine_table[idx];
}

void led_entry(void *parameter)
{
    rt_tick_t cycle_start = rt_tick_get();
    int fixed_level = 0;

    LED_OFF();

    while (1)
    {
        if (!led_running)
        {
            LED_OFF();
            current_brightness = 0;
            rt_thread_mdelay(blink_interval);
            cycle_start = rt_tick_get();
            continue;
        }

        if (current_mode == LED_MODE_GPIO)
        {
            LED_ON();
            rt_thread_mdelay(blink_interval);
            LED_OFF();
            rt_thread_mdelay(blink_interval);
        }
        else
        {
            if (target_brightness != 0)
            {
                if (target_brightness != current_brightness)
                {
                    fixed_level = (int)((rt_uint32_t)target_brightness * LED_BREATHE_LEVELS / 100);
                    current_brightness = target_brightness;
                }
                rt_uint8_t pwm_duty = breathe_duty((rt_uint8_t)fixed_level);
                led_sw_pwm_output(pwm_duty);
            }
            else
            {
                rt_tick_t now = rt_tick_get();
                rt_uint32_t elapsed_ms = (rt_uint32_t)(now - cycle_start) * 1000 / RT_TICK_PER_SECOND;
                rt_uint32_t period = breathe_period;
                rt_uint32_t pos = elapsed_ms % period;
                int level;

                if (pos < period / 2)
                    level = (int)(pos * LED_BREATHE_LEVELS / (period / 2));
                else
                    level = (int)((period - pos) * LED_BREATHE_LEVELS / (period / 2));

                if (level >= LED_BREATHE_LEVELS)
                    level = LED_BREATHE_LEVELS;
                if (level < 0)
                    level = 0;

                current_brightness = (rt_uint8_t)((rt_uint32_t)level * 100 / LED_BREATHE_LEVELS);

                rt_uint8_t sine_val = breathe_lookup(level);
                rt_uint8_t pwm_duty = breathe_duty(sine_val);
                led_sw_pwm_output(pwm_duty);
            }
        }
    }
}

static void led_print_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  led on                    Turn on LED (blink/breathe by mode)\n");
    rt_kprintf("  led off                   Turn off LED\n");
    rt_kprintf("  led blink [ms]            Set blink interval (50~5000ms)\n");
    rt_kprintf("  led breathe [ms]          Set breathe period (500~10000ms)\n");
    rt_kprintf("  led mode gpio             Switch to GPIO blink mode\n");
    rt_kprintf("  led mode pwm              Switch to PWM breathe mode\n");
    rt_kprintf("  led brightness <0~100>    Set brightness (PWM mode)\n");
    rt_kprintf("  led -h                    Show this help\n");
}

static int led_cmd(int argc, char *argv[])
{
    if (argc < 2)
    {
        led_print_usage();
        return -RT_ERROR;
    }

    if (rt_strcmp(argv[1], "-h") == 0)
    {
        led_print_usage();
        return RT_EOK;
    }

    if (rt_strcmp(argv[1], "on") == 0)
    {
        led_on();
    }
    else if (rt_strcmp(argv[1], "off") == 0)
    {
        led_off();
    }
    else if (rt_strcmp(argv[1], "blink") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("Current blink interval: %d ms\n", blink_interval);
            rt_kprintf("Usage: led blink <50~5000 ms>\n");
            return -RT_ERROR;
        }

        int interval = atoi(argv[2]);
        if (interval < 50 || interval > 5000)
        {
            rt_kprintf("Interval out of range (50~5000 ms)\n");
            return -RT_ERROR;
        }
        led_set_blink_interval((rt_uint32_t)interval);
        current_mode = LED_MODE_GPIO;
        led_running = 1;
        rt_kprintf("LED blink interval: %d ms\n", interval);
    }
    else if (rt_strcmp(argv[1], "breathe") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("Current breathe period: %d ms\n", breathe_period);
            rt_kprintf("Usage: led breathe <500~10000 ms>\n");
            return -RT_ERROR;
        }

        int period = atoi(argv[2]);
        if (period < 500 || period > 10000)
        {
            rt_kprintf("Period out of range (500~10000 ms)\n");
            return -RT_ERROR;
        }
        led_set_breathe_period((rt_uint32_t)period);
        current_mode = LED_MODE_PWM;
        target_brightness = 0;
        led_running = 1;
        rt_kprintf("LED breathe period: %d ms\n", period);
    }
    else if (rt_strcmp(argv[1], "mode") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("Current mode: %s\n", current_mode == LED_MODE_GPIO ? "gpio" : "pwm");
            rt_kprintf("Usage: led mode <gpio|pwm>\n");
            return -RT_ERROR;
        }

        if (rt_strcmp(argv[2], "gpio") == 0)
        {
            led_set_mode(LED_MODE_GPIO);
            rt_kprintf("LED mode: gpio\n");
        }
        else if (rt_strcmp(argv[2], "pwm") == 0)
        {
            led_set_mode(LED_MODE_PWM);
            rt_kprintf("LED mode: pwm\n");
        }
        else
        {
            rt_kprintf("Unknown mode: %s (gpio|pwm)\n", argv[2]);
            return -RT_ERROR;
        }
    }
    else if (rt_strcmp(argv[1], "brightness") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("Current brightness: %d%%\n", current_brightness);
            rt_kprintf("Usage: led brightness <0~100>\n");
            return -RT_ERROR;
        }

        int brightness = atoi(argv[2]);
        if (brightness < 0)
            brightness = 0;
        if (brightness > 100)
            brightness = 100;
        led_set_brightness((rt_uint8_t)brightness);
        rt_kprintf("LED brightness: %d%%\n", brightness);
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
        led_print_usage();
        return -RT_ERROR;
    }

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(led_cmd, led, led control : on / off / blink / breathe / mode / brightness);
