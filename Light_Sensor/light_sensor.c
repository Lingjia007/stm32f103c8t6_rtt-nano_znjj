#include "light_sensor.h"
#include "adc.h"
#include "rtthread.h"

extern ADC_HandleTypeDef hadc1;

void light_sensor_init(void)
{
}

uint16_t light_sensor_read_raw(void)
{
    uint16_t adc_value = 0;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    return adc_value;
}

uint8_t light_sensor_read_percentage(void)
{
    uint16_t adc_value;
    uint8_t percentage;

    adc_value = light_sensor_read_raw();

    percentage = (uint8_t)(100 - (adc_value * 100) / 4095);

    return percentage;
}
