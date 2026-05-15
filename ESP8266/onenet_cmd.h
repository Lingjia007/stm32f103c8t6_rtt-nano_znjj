#ifndef ONENET_CMD_H
#define ONENET_CMD_H

#include "platform_mqtt.h"
#include "onenet_kv.h"
#include <stdint.h>

typedef struct
{
    onenet_kv_table_t *kv_table;
    platform_mqtt_base_t *mqtt;
    uint8_t link_id;
    const char *product_id;
    const char *device_name;
} onenet_cmd_ctx_t;

void onenet_cmd_init(onenet_cmd_ctx_t *ctx,
                     onenet_kv_table_t *kv_table,
                     platform_mqtt_base_t *mqtt,
                     uint8_t link_id,
                     const char *product_id,
                     const char *device_name);

int8_t onenet_cmd_process_set(onenet_cmd_ctx_t *ctx,
                              const char *payload);

int8_t onenet_cmd_poll(onenet_cmd_ctx_t *ctx);

#endif
