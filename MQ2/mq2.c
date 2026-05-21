#include "mq2.h"
#include "adc.h"
#include "rtthread.h"

extern ADC_HandleTypeDef hadc1;

void mq2_init(void)
{
}

uint16_t mq2_read_raw(void)
{
    uint16_t adc_value = 0;
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    return adc_value;
}

uint8_t mq2_read_percentage(void)
{
    uint16_t adc_value;
    uint8_t percentage;

    adc_value = mq2_read_raw();

    percentage = (uint8_t)((adc_value * 100) / 4095);

    return percentage;
}
