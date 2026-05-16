#include "onenet_kv.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void onenet_kv_init(onenet_kv_table_t *table)
{
    if (table == NULL)
        return;
    memset(table, 0, sizeof(onenet_kv_table_t));
}

int8_t onenet_kv_register(onenet_kv_table_t *table,
                           const char *key,
                           uint8_t value_type,
                           void *value_ptr,
                           onenet_kv_on_change_t on_change)
{
    if (table == NULL || key == NULL || value_ptr == NULL)
        return -1;

    if (table->count >= ONENET_KV_MAX_ENTRIES)
        return -2;

    onenet_kv_entry_t *existing = onenet_kv_find(table, key);
    if (existing != NULL)
        return -3;

    onenet_kv_entry_t *entry = &table->entries[table->count];
    strncpy(entry->key, key, ONENET_KV_MAX_KEY_LEN - 1);
    entry->key[ONENET_KV_MAX_KEY_LEN - 1] = '\0';
    entry->value_type = value_type;
    entry->value_ptr = value_ptr;
    entry->dirty = 0;
    entry->on_change = on_change;

    table->count++;
    return 0;
}

onenet_kv_entry_t *onenet_kv_find(onenet_kv_table_t *table, const char *key)
{
    if (table == NULL || key == NULL)
        return NULL;

    for (uint8_t i = 0; i < table->count; i++)
    {
        if (strcmp(table->entries[i].key, key) == 0)
            return &table->entries[i];
    }
    return NULL;
}

int8_t onenet_kv_set_value(onenet_kv_table_t *table,
                            const char *key,
                            const char *value_str)
{
    if (table == NULL || key == NULL || value_str == NULL)
        return -1;

    onenet_kv_entry_t *entry = onenet_kv_find(table, key);
    if (entry == NULL)
        return -2;

    switch (entry->value_type)
    {
    case PLATFORM_MQTT_VALUE_INT:
        *((int *)entry->value_ptr) = atoi(value_str);
        break;
    case PLATFORM_MQTT_VALUE_FLOAT:
        return -3;
    case PLATFORM_MQTT_VALUE_BOOL:
        if (strcmp(value_str, "true") == 0 || strcmp(value_str, "1") == 0)
            *((uint8_t *)entry->value_ptr) = 1;
        else
            *((uint8_t *)entry->value_ptr) = 0;
        break;
    case PLATFORM_MQTT_VALUE_STRING:
        strncpy((char *)entry->value_ptr, value_str, ONENET_KV_MAX_STRING_LEN - 1);
        ((char *)entry->value_ptr)[ONENET_KV_MAX_STRING_LEN - 1] = '\0';
        break;
    default:
        *((int *)entry->value_ptr) = atoi(value_str);
        break;
    }

    entry->dirty = 1;

    if (entry->on_change != NULL)
        entry->on_change(key, entry->value_ptr, entry->value_type);

    return 0;
}

int8_t onenet_kv_get_property(onenet_kv_table_t *table,
                               const char *key,
                               platform_mqtt_property_t *prop)
{
    if (table == NULL || key == NULL || prop == NULL)
        return -1;

    onenet_kv_entry_t *entry = onenet_kv_find(table, key);
    if (entry == NULL)
        return -2;

    memset(prop, 0, sizeof(platform_mqtt_property_t));
    strncpy(prop->key, entry->key, sizeof(prop->key) - 1);
    prop->value_type = entry->value_type;

    switch (entry->value_type)
    {
    case PLATFORM_MQTT_VALUE_INT:
        prop->value_int = *((int *)entry->value_ptr);
        break;
    case PLATFORM_MQTT_VALUE_FLOAT:
        return -3;
    case PLATFORM_MQTT_VALUE_BOOL:
        prop->value_int = *((uint8_t *)entry->value_ptr) ? 1 : 0;
        break;
    case PLATFORM_MQTT_VALUE_STRING:
        strncpy(prop->id, (char *)entry->value_ptr, sizeof(prop->id) - 1);
        break;
    default:
        prop->value_int = *((int *)entry->value_ptr);
        break;
    }

    return 0;
}

int8_t onenet_kv_get_all_properties(onenet_kv_table_t *table,
                                     platform_mqtt_property_t *props,
                                     uint8_t max_props)
{
    if (table == NULL || props == NULL)
        return -1;

    uint8_t count = (table->count < max_props) ? table->count : max_props;

    for (uint8_t i = 0; i < count; i++)
    {
        onenet_kv_get_property(table, table->entries[i].key, &props[i]);
    }

    return (int8_t)count;
}

static char *find_json_value(const char *json, const char *key, char *buf, uint16_t buf_len)
{
    char search_pattern[ONENET_KV_MAX_KEY_LEN + 8];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);

    char *key_pos = strstr(json, search_pattern);
    if (key_pos == NULL)
        return NULL;

    char *colon = strchr(key_pos + strlen(search_pattern), ':');
    if (colon == NULL)
        return NULL;

    colon++;
    while (*colon == ' ')
        colon++;

    if (*colon == '"')
    {
        colon++;
        char *end = strchr(colon, '"');
        if (end == NULL)
            return NULL;
        uint16_t len = end - colon;
        if (len >= buf_len)
            len = buf_len - 1;
        strncpy(buf, colon, len);
        buf[len] = '\0';
        return buf;
    }
    else if (*colon == '{')
    {
        char *inner_key = strstr(colon, "\"value\"");
        if (inner_key == NULL)
        {
            inner_key = strstr(colon, "\"v\"");
            if (inner_key == NULL)
                return NULL;
        }

        char *inner_colon = strchr(inner_key, ':');
        if (inner_colon == NULL)
            return NULL;

        inner_colon++;
        while (*inner_colon == ' ')
            inner_colon++;

        if (*inner_colon == '"')
        {
            inner_colon++;
            char *end = strchr(inner_colon, '"');
            if (end == NULL)
                return NULL;
            uint16_t len = end - inner_colon;
            if (len >= buf_len)
                len = buf_len - 1;
            strncpy(buf, inner_colon, len);
            buf[len] = '\0';
            return buf;
        }
        else
        {
            char *end = inner_colon;
            while (*end != ',' && *end != '}' && *end != '\0')
                end++;
            uint16_t len = end - inner_colon;
            if (len >= buf_len)
                len = buf_len - 1;
            strncpy(buf, inner_colon, len);
            buf[len] = '\0';
            return buf;
        }
    }
    else
    {
        char *end = colon;
        while (*end != ',' && *end != '}' && *end != '\0')
            end++;
        uint16_t len = end - colon;
        if (len >= buf_len)
            len = buf_len - 1;
        strncpy(buf, colon, len);
        buf[len] = '\0';
        return buf;
    }
}

int8_t onenet_kv_parse_set_payload(onenet_kv_table_t *table,
                                    const char *payload)
{
    if (table == NULL || payload == NULL)
        return -1;

    char *params_start = strstr(payload, "\"params\"");
    if (params_start == NULL)
        return -2;

    char *params_obj = strchr(params_start, '{');
    if (params_obj == NULL)
        return -2;

    int8_t updated = 0;
    char value_buf[64];

    for (uint8_t i = 0; i < table->count; i++)
    {
        if (find_json_value(params_obj, table->entries[i].key, value_buf, sizeof(value_buf)) != NULL)
        {
            if (onenet_kv_set_value(table, table->entries[i].key, value_buf) == 0)
            {
                updated++;
            }
        }
    }

    return updated;
}

void onenet_kv_mark_clean(onenet_kv_table_t *table, const char *key)
{
    if (table == NULL || key == NULL)
        return;

    onenet_kv_entry_t *entry = onenet_kv_find(table, key);
    if (entry != NULL)
        entry->dirty = 0;
}

void onenet_kv_mark_all_clean(onenet_kv_table_t *table)
{
    if (table == NULL)
        return;

    for (uint8_t i = 0; i < table->count; i++)
        table->entries[i].dirty = 0;
}

uint8_t onenet_kv_has_dirty(onenet_kv_table_t *table)
{
    if (table == NULL)
        return 0;

    for (uint8_t i = 0; i < table->count; i++)
    {
        if (table->entries[i].dirty)
            return 1;
    }
    return 0;
}
