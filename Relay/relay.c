#include "relay.h"

static uint8_t g_fan_state = RELAY_OFF;
static uint8_t g_nebulizer_state = RELAY_OFF;

void relay_init(void)
{
    HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NEBULIZER_GPIO_Port, NEBULIZER_Pin, GPIO_PIN_RESET);
    g_fan_state = RELAY_OFF;
    g_nebulizer_state = RELAY_OFF;
}

void relay_fan_set(uint8_t state)
{
    if (state == RELAY_ON)
    {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
        g_fan_state = RELAY_ON;
    }
    else
    {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
        g_fan_state = RELAY_OFF;
    }
}

void relay_nebulizer_set(uint8_t state)
{
    if (state == RELAY_ON)
    {
        HAL_GPIO_WritePin(NEBULIZER_GPIO_Port, NEBULIZER_Pin, GPIO_PIN_SET);
        g_nebulizer_state = RELAY_ON;
    }
    else
    {
        HAL_GPIO_WritePin(NEBULIZER_GPIO_Port, NEBULIZER_Pin, GPIO_PIN_RESET);
        g_nebulizer_state = RELAY_OFF;
    }
}

uint8_t relay_fan_get(void)
{
    return g_fan_state;
}

uint8_t relay_nebulizer_get(void)
{
    return g_nebulizer_state;
}
