#ifndef DHT11_H
#define DHT11_H

#include "main.h"

#define DHT11_OK       0
#define DHT11_ERROR    (-1)
#define DHT11_TIMEOUT  (-2)

typedef struct
{
    uint8_t humidity_int;
    uint8_t humidity_deci;
    uint8_t temperature_int;
    uint8_t temperature_deci;
    uint8_t check_sum;
} dht11_data_t;

int dht11_read(dht11_data_t *data);
int dht11_get_temperature(void);
int dht11_get_humidity(void);

#endif
