#ifndef BUZZER_H
#define BUZZER_H

#include "main.h"

#define BUZZER_OFF 0
#define BUZZER_ON 1

void buzzer_init(void);
void buzzer_set(uint8_t state);
uint8_t buzzer_get(void);

#endif
