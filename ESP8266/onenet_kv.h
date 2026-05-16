#ifndef ONENET_KV_H
#define ONENET_KV_H

#include "platform_mqtt.h"
#include <stdint.h>
#include <stddef.h>

#define ONENET_KV_MAX_KEY_LEN 16
#define ONENET_KV_MAX_ENTRIES 8
#define ONENET_KV_MAX_STRING_LEN 16

typedef void (*onenet_kv_on_change_t)(const char *key, void *value, uint8_t value_type);

typedef struct
{
    char key[ONENET_KV_MAX_KEY_LEN];
    uint8_t value_type;
    void *value_ptr;
    uint8_t dirty;
    onenet_kv_on_change_t on_change;
} onenet_kv_entry_t;

typedef struct
{
    onenet_kv_entry_t entries[ONENET_KV_MAX_ENTRIES];
    uint8_t count;
} onenet_kv_table_t;

void onenet_kv_init(onenet_kv_table_t *table);

int8_t onenet_kv_register(onenet_kv_table_t *table,
                           const char *key,
                           uint8_t value_type,
                           void *value_ptr,
                           onenet_kv_on_change_t on_change);

onenet_kv_entry_t *onenet_kv_find(onenet_kv_table_t *table, const char *key);

int8_t onenet_kv_set_value(onenet_kv_table_t *table,
                            const char *key,
                            const char *value_str);

int8_t onenet_kv_get_property(onenet_kv_table_t *table,
                               const char *key,
                               platform_mqtt_property_t *prop);

int8_t onenet_kv_get_all_properties(onenet_kv_table_t *table,
                                     platform_mqtt_property_t *props,
                                     uint8_t max_props);

int8_t onenet_kv_parse_set_payload(onenet_kv_table_t *table,
                                    const char *payload);

void onenet_kv_mark_clean(onenet_kv_table_t *table, const char *key);

void onenet_kv_mark_all_clean(onenet_kv_table_t *table);

uint8_t onenet_kv_has_dirty(onenet_kv_table_t *table);

#endif
