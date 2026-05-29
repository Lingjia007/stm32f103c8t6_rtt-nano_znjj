#include "buzzer.h"

static uint8_t g_buzzer_state = BUZZER_OFF;

void buzzer_init(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
    g_buzzer_state = BUZZER_OFF;
}

void buzzer_set(uint8_t state)
{
    if (state == BUZZER_ON)
    {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
        g_buzzer_state = BUZZER_ON;
    }
    else
    {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
        g_buzzer_state = BUZZER_OFF;
    }
}

uint8_t buzzer_get(void)
{
    return g_buzzer_state;
}
