#include "mqtt_worker.h"
#include <string.h>
#include <stdio.h>

void mqtt_worker_init(mqtt_worker_t *w,
                      onenet_kv_table_t *kv_table,
                      platform_mqtt_base_t *mqtt,
                      uint8_t link_id,
                      const char *product_id,
                      const char *device_name,
                      rt_mutex_t esp8266_mutex)
{
    if (w == NULL)
        return;

    memset(w, 0, sizeof(mqtt_worker_t));
    w->kv_table = kv_table;
    w->mqtt = mqtt;
    w->link_id = link_id;
    w->product_id = product_id;
    w->device_name = device_name;
    w->esp8266_mutex = esp8266_mutex;

    w->notify = rt_sem_create("wk_sem", 0, RT_IPC_FLAG_PRIO);
    rt_mutex_init(&w->lock, "wk_lck", RT_IPC_FLAG_PRIO);
}

int8_t mqtt_worker_submit(mqtt_worker_t *w, const mqtt_post_item_t *item)
{
    if (w == NULL || item == NULL)
        return -1;

    rt_mutex_take(&w->lock, RT_WAITING_FOREVER);

    if (w->count >= MQTT_WORK_QUEUE_SIZE)
    {
        rt_mutex_release(&w->lock);
        return -2;
    }

    for (uint8_t i = 0; i < w->count; i++)
    {
        for (uint8_t k = 0; k < item->key_count; k++)
        {
            if (w->items[i].key_count < MQTT_WORK_MAX_KEYS)
            {
                strncpy(w->items[i].keys[w->items[i].key_count],
                        item->keys[k], ONENET_KV_MAX_KEY_LEN - 1);
                w->items[i].key_count++;
            }
        }
        rt_mutex_release(&w->lock);
        rt_sem_release(w->notify);
        return 0;
    }

    w->items[w->count] = *item;
    w->count++;

    rt_mutex_release(&w->lock);
    rt_sem_release(w->notify);
    return 0;
}

static int8_t worker_dequeue(mqtt_worker_t *w, mqtt_post_item_t *item)
{
    rt_mutex_take(&w->lock, RT_WAITING_FOREVER);

    if (w->count == 0)
    {
        rt_mutex_release(&w->lock);
        return -1;
    }

    *item = w->items[0];

    for (uint8_t i = 0; i < w->count - 1; i++)
        w->items[i] = w->items[i + 1];
    w->count--;

    rt_mutex_release(&w->lock);
    return 0;
}

static void worker_process_item(mqtt_worker_t *w, const mqtt_post_item_t *item)
{
    static platform_mqtt_property_t props[MQTT_WORK_MAX_KEYS];
    memset(props, 0, sizeof(props));

    uint8_t prop_count = 0;

    for (uint8_t i = 0; i < item->key_count; i++)
    {
        if (onenet_kv_get_property(w->kv_table, item->keys[i],
                                   &props[prop_count]) == 0)
        {
            prop_count++;
        }
    }

    if (prop_count > 0)
    {
        if (w->esp8266_mutex != RT_NULL)
            rt_mutex_take(w->esp8266_mutex, RT_WAITING_FOREVER);

        int16_t ret = MQTT_PUBLISH_PROPERTY(w->mqtt, w->link_id,
                                            w->product_id, w->device_name,
                                            props, prop_count, "001");

        if (w->esp8266_mutex != RT_NULL)
            rt_mutex_release(w->esp8266_mutex);

        if (ret == PLATFORM_MQTT_OK)
            rt_kprintf("Worker: POST OK (%d props)\n", prop_count);
        else
            rt_kprintf("Worker: POST FAIL (err=%d)\n", ret);
    }
}

uint8_t mqtt_worker_process_one(mqtt_worker_t *w)
{
    mqtt_post_item_t item;
    if (worker_dequeue(w, &item) != 0)
        return 0;
    worker_process_item(w, &item);
    return 1;
}

void mqtt_worker_process_all(mqtt_worker_t *w)
{
    mqtt_post_item_t item;
    while (worker_dequeue(w, &item) == 0)
    {
        worker_process_item(w, &item);
    }
}

void mqtt_worker_thread_entry(void *parameter)
{
    mqtt_worker_t *w = (mqtt_worker_t *)parameter;
    mqtt_post_item_t item;

    rt_kprintf("[mqtt_worker] Started\n");

    while (1)
    {
        rt_sem_take(w->notify, RT_WAITING_FOREVER);

        while (worker_dequeue(w, &item) == 0)
        {
            worker_process_item(w, &item);
        }
    }
}
