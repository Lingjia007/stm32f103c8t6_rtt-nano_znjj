#include "dht11.h"
#include "rtthread.h"
#include "rthw.h"

static void dht11_set_output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

static void dht11_set_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

static void dht11_write_pin(GPIO_PinState state)
{
    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, state);
}

static GPIO_PinState dht11_read_pin(void)
{
    return HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin);
}

static uint8_t dht11_wait_pin(GPIO_PinState expected, uint32_t timeout_us)
{
    while (dht11_read_pin() != expected)
    {
        if (timeout_us == 0)
            return 0;
        timeout_us--;
        rt_hw_us_delay(1);
    }
    return 1;
}

static int dht11_read_byte(uint8_t *byte)
{
    uint8_t i;

    *byte = 0;

    for (i = 0; i < 8; i++)
    {
        if (!dht11_wait_pin(GPIO_PIN_SET, 100))
            return DHT11_TIMEOUT;

        rt_hw_us_delay(40);

        if (dht11_read_pin() == GPIO_PIN_SET)
        {
            *byte |= (1 << (7 - i));
            if (!dht11_wait_pin(GPIO_PIN_RESET, 100))
                return DHT11_TIMEOUT;
        }
    }

    return DHT11_OK;
}

int dht11_read(dht11_data_t *data)
{
    uint8_t buf[5];
    int i;

    dht11_set_output();
    dht11_write_pin(GPIO_PIN_RESET);
    rt_thread_mdelay(20);
    dht11_write_pin(GPIO_PIN_SET);
    rt_hw_us_delay(30);

    dht11_set_input();

    if (!dht11_wait_pin(GPIO_PIN_RESET, 100))
        return DHT11_TIMEOUT;

    if (!dht11_wait_pin(GPIO_PIN_SET, 100))
        return DHT11_TIMEOUT;

    if (!dht11_wait_pin(GPIO_PIN_RESET, 100))
        return DHT11_TIMEOUT;

    for (i = 0; i < 5; i++)
    {
        if (dht11_read_byte(&buf[i]) != DHT11_OK)
            return DHT11_TIMEOUT;
    }

    dht11_set_output();
    dht11_write_pin(GPIO_PIN_SET);

    if (buf[4] != (uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]))
        return DHT11_ERROR;

    data->humidity_int = buf[0];
    data->humidity_deci = buf[1];
    data->temperature_int = buf[2];
    data->temperature_deci = buf[3];
    data->check_sum = buf[4];

    return DHT11_OK;
}

int dht11_get_temperature(void)
{
    dht11_data_t data;
    if (dht11_read(&data) != DHT11_OK)
        return DHT11_ERROR;
    return data.temperature_int;
}

int dht11_get_humidity(void)
{
    dht11_data_t data;
    if (dht11_read(&data) != DHT11_OK)
        return DHT11_ERROR;
    return data.humidity_int;
}
