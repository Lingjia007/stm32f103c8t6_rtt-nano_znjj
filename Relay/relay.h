#ifndef RELAY_H
#define RELAY_H

#include "main.h"

#define RELAY_OFF 0
#define RELAY_ON 1

void relay_init(void);
void relay_fan_set(uint8_t state);
void relay_nebulizer_set(uint8_t state);
uint8_t relay_fan_get(void);
uint8_t relay_nebulizer_get(void);

#endif
