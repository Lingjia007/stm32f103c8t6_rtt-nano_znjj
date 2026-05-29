#include "onenet_cmd.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>

void onenet_cmd_init(onenet_cmd_ctx_t *ctx,
                     onenet_kv_table_t *kv_table)
{
    if (ctx == NULL)
        return;

    ctx->kv_table = kv_table;

    rt_mutex_init(&ctx->lock, "cmd_mtx", RT_IPC_FLAG_PRIO);
}

int8_t onenet_cmd_handle_set(onenet_cmd_ctx_t *ctx,
                             const char *payload,
                             int *reply_code,
                             const char **reply_msg)
{
    if (ctx == NULL || payload == NULL)
        return -1;

    rt_mutex_take(&ctx->lock, RT_WAITING_FOREVER);

    int8_t cb_result = 0;
    int8_t updated = onenet_kv_parse_set_payload(ctx->kv_table, payload, &cb_result);

    if (reply_code != NULL)
    {
        if (updated < 0)
            *reply_code = 500;
        else if (cb_result < 0)
            *reply_code = 501;
        else
            *reply_code = 200;
    }

    if (reply_msg != NULL)
    {
        if (updated < 0)
            *reply_msg = "parse_error";
        else if (cb_result < 0)
            *reply_msg = "callback_error";
        else
            *reply_msg = "user_succ";
    }

    rt_kprintf("OneNet SET: %d updated, cb=%d, reply=%d\n",
               updated, cb_result, reply_code ? *reply_code : 0);

    rt_mutex_release(&ctx->lock);
    return updated;
}

int8_t onenet_cmd_handle_get(onenet_cmd_ctx_t *ctx,
                             const char *payload,
                             const char *msg_id)
{
    if (ctx == NULL || payload == NULL)
        return -1;

    rt_kprintf("OneNet GET: %s\n", payload);
    return 0;
}
