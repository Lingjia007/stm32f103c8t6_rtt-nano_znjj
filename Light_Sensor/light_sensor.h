#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include "main.h"

void light_sensor_init(void);
uint16_t light_sensor_read_raw(void);
uint8_t light_sensor_read_percentage(void);

#endif
