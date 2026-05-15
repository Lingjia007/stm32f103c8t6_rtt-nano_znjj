#include "pir_sensor.h"
#include "rtthread.h"

void pir_sensor_init(void)
{
}

uint8_t pir_sensor_read(void)
{
    GPIO_PinState pin_state;

    pin_state = HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN);

    if (pin_state == GPIO_PIN_RESET)
    {
        return PIR_DETECTED;
    }
    else
    {
        return PIR_NOT_DETECTED;
    }
}
