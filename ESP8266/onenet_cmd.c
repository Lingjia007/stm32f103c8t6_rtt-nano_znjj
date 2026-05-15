#include "onenet_cmd.h"
#include "esp8266_config.h"
#include <string.h>
#include <stdio.h>
#include <rtthread.h>

void onenet_cmd_init(onenet_cmd_ctx_t *ctx,
                      onenet_kv_table_t *kv_table,
                      platform_mqtt_base_t *mqtt,
                      uint8_t link_id,
                      const char *product_id,
                      const char *device_name)
{
    if (ctx == NULL)
        return;

    ctx->kv_table = kv_table;
    ctx->mqtt = mqtt;
    ctx->link_id = link_id;
    ctx->product_id = product_id;
    ctx->device_name = device_name;
}

int8_t onenet_cmd_process_set(onenet_cmd_ctx_t *ctx,
                               const char *payload)
{
    if (ctx == NULL || payload == NULL)
        return -1;

    int8_t updated = onenet_kv_parse_set_payload(ctx->kv_table, payload);

    char msg_id[32] = {0};
    char *id_start = strstr(payload, "\"id\"");
    if (id_start != NULL)
    {
        char *colon = strchr(id_start, ':');
        if (colon != NULL)
        {
            colon++;
            while (*colon == ' ' || *colon == '"')
                colon++;
            char *end = colon;
            while (*end != '"' && *end != ',' && *end != '}' && *end != '\0')
                end++;
            uint16_t len = end - colon;
            if (len >= 32)
                len = 31;
            strncpy(msg_id, colon, len);
            msg_id[len] = '\0';
        }
    }

    if (ctx->mqtt != NULL)
    {
        MQTT_PUBLISH_SET_REPLY(ctx->mqtt, ctx->link_id,
                               ctx->product_id, ctx->device_name,
                               msg_id, 200, "user_succ");
    }

    rt_kprintf("OneNet SET processed: %d properties updated\n", updated);
    return updated;
}

int8_t onenet_cmd_poll(onenet_cmd_ctx_t *ctx)
{
    if (ctx == NULL || ctx->mqtt == NULL)
        return -1;

    static char recv_topic[PLATFORM_MQTT_MAX_TOPIC_LEN];
    static char recv_payload[PLATFORM_MQTT_MAX_PAYLOAD_LEN];
    static char recv_msg_id[32];

    memset(recv_topic, 0, sizeof(recv_topic));
    memset(recv_payload, 0, sizeof(recv_payload));
    memset(recv_msg_id, 0, sizeof(recv_msg_id));

    int16_t recv_ret = MQTT_CHECK_PROPERTY_SET_RECV(ctx->mqtt,
                                                     recv_topic, recv_payload,
                                                     sizeof(recv_payload), recv_msg_id);
    if (recv_ret != PLATFORM_MQTT_OK)
        return 0;

    if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_POST_REPLY) == 0)
        return 0;

    if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_SET) == 0)
    {
        rt_kprintf("OneNet SET: %s\n", recv_payload);
        return onenet_cmd_process_set(ctx, recv_payload);
    }

    if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_GET) == 0)
    {
        rt_kprintf("OneNet GET: %s\n", recv_payload);
        return 0;
    }

    if (strcmp(recv_topic, ONENET_TOPIC_OTA_INFORM) == 0)
    {
        rt_kprintf("OneNet OTA: %s\n", recv_payload);
        return 0;
    }

    return 0;
}
