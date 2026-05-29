#ifndef ONENET_CMD_H
#define ONENET_CMD_H

#include "onenet_kv.h"
#include <stdint.h>
#include <rtthread.h>

typedef struct
{
    onenet_kv_table_t *kv_table;
    struct rt_mutex lock;
} onenet_cmd_ctx_t;

void onenet_cmd_init(onenet_cmd_ctx_t *ctx,
                     onenet_kv_table_t *kv_table);

int8_t onenet_cmd_handle_set(onenet_cmd_ctx_t *ctx,
                             const char *payload,
                             int *reply_code,
                             const char **reply_msg);

int8_t onenet_cmd_handle_get(onenet_cmd_ctx_t *ctx,
                             const char *payload,
                             const char *msg_id);

#endif
