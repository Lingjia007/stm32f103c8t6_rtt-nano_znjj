#ifndef MQTT_WORKER_H
#define MQTT_WORKER_H

#include "platform_mqtt.h"
#include "onenet_kv.h"
#include <stdint.h>
#include <rtthread.h>

#define MQTT_WORK_QUEUE_SIZE 4
#define MQTT_WORK_MAX_KEYS 4

typedef struct
{
    char keys[MQTT_WORK_MAX_KEYS][ONENET_KV_MAX_KEY_LEN];
    uint8_t key_count;
} mqtt_post_item_t;

typedef struct
{
    mqtt_post_item_t items[MQTT_WORK_QUEUE_SIZE];
    uint8_t count;
    rt_sem_t notify;
    struct rt_mutex lock;
    onenet_kv_table_t *kv_table;
    platform_mqtt_base_t *mqtt;
    uint8_t link_id;
    const char *product_id;
    const char *device_name;
    rt_mutex_t esp8266_mutex;
} mqtt_worker_t;

void mqtt_worker_init(mqtt_worker_t *w,
                      onenet_kv_table_t *kv_table,
                      platform_mqtt_base_t *mqtt,
                      uint8_t link_id,
                      const char *product_id,
                      const char *device_name,
                      rt_mutex_t esp8266_mutex);

int8_t mqtt_worker_submit(mqtt_worker_t *w, const mqtt_post_item_t *item);

void mqtt_worker_thread_entry(void *parameter);

#endif
