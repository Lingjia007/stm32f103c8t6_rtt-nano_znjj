#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include "main.h"

#define PIR_PIN GPIO_PIN_8
#define PIR_PORT GPIOA

#define PIR_DETECTED    1
#define PIR_NOT_DETECTED 0

void pir_sensor_init(void);
uint8_t pir_sensor_read(void);

#endif
