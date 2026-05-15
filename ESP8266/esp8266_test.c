#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp8266_init.h"
#include "esp8266_config.h"
#include "dht11.h"
#include "onenet_kv.h"
#include "main.h"

#if ESP8266_TEST_ENABLED && defined(RT_USING_FINSH)
#include <finsh.h>

static int esp8266_at_test(void)
{
    rt_kprintf("Testing ESP8266 AT command...\n");

    int16_t ret = WIFI_AT_TEST(&g_esp8266_wifi.base);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("AT test success! ESP8266 is responding.\n");
        return 0;
    }
    else
    {
        rt_kprintf("AT test failed! Error code: %d\n", ret);
        rt_kprintf("Please check:\n");
        rt_kprintf("  1. ESP8266 power supply\n");
        rt_kprintf("  2. UART1 TX(PA9)/RX(PA10) connections\n");
        rt_kprintf("  3. ESP8266 baud rate (should be 115200)\n");
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_at_test, Test ESP8266 AT command);

static int esp8266_set_sta(int argc, char **argv)
{
    rt_kprintf("Setting ESP8266 to Station mode...\n");

    int16_t ret = WIFI_SET_MODE(&g_esp8266_wifi.base, PLATFORM_WIFI_MODE_STA);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("Set Station mode success!\n");
        return 0;
    }
    else
    {
        rt_kprintf("Set Station mode failed! Error code: %d\n", ret);
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_set_sta, Set ESP8266 to Station mode);

static int esp8266_join_ap(int argc, char **argv)
{
    const char *ssid = ESP8266_WIFI_SSID;
    const char *pwd = ESP8266_WIFI_PASSWORD;

    if (argc >= 3)
    {
        ssid = argv[1];
        pwd = argv[2];
    }
    else if (argc == 1)
    {
        rt_kprintf("Using default SSID from config: %s\n", ssid);
    }
    else
    {
        rt_kprintf("Usage: esp8266_join_ap [ssid] [password]\n");
        rt_kprintf("       or use default config in esp8266_config.h\n");
        return -1;
    }

    rt_kprintf("Connecting to WiFi: %s\n", ssid);
    rt_kprintf("Please wait...\n");

    int16_t ret = WIFI_JOIN_AP(&g_esp8266_wifi.base, ssid, pwd);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("WiFi connected successfully!\n");
        return 0;
    }
    else
    {
        rt_kprintf("WiFi connection failed! Error code: %d\n", ret);
        rt_kprintf("Please check:\n");
        rt_kprintf("  1. SSID and password are correct\n");
        rt_kprintf("  2. WiFi signal is strong enough\n");
        rt_kprintf("  3. Router is working properly\n");
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_join_ap, Connect to WiFi AP);

static int esp8266_get_ip(void)
{
    char ip_buf[64] = {0};

    rt_kprintf("Getting IP address...\n");

    int16_t ret = WIFI_GET_IP(&g_esp8266_wifi.base, ip_buf, sizeof(ip_buf));

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("IP Address: %s\n", ip_buf);
        return 0;
    }
    else
    {
        rt_kprintf("Get IP failed! Error code: %d\n", ret);
        rt_kprintf("Please connect to WiFi first.\n");
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_get_ip, Get ESP8266 IP address);

static int esp8266_reset(void)
{
    rt_kprintf("Resetting ESP8266...\n");

    int16_t ret = WIFI_SW_RESET(&g_esp8266_wifi.base);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("ESP8266 reset success!\n");
        rt_kprintf("Please wait for ESP8266 to restart...\n");
        return 0;
    }
    else
    {
        rt_kprintf("ESP8266 reset failed! Error code: %d\n", ret);
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_reset, Software reset ESP8266);

static int esp8266_tcp_connect(int argc, char **argv)
{
    if (argc < 3)
    {
        rt_kprintf("Usage: esp8266_tcp_connect <host> <port>\n");
        rt_kprintf("Example: esp8266_tcp_connect 192.168.1.100 8080\n");
        return -1;
    }

    const char *host = argv[1];
    uint16_t port = (uint16_t)atoi(argv[2]);

    rt_kprintf("Connecting to TCP server: %s:%d\n", host, port);

    int16_t ret = WIFI_CONNECT_TCP(&g_esp8266_wifi.base, host, port);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("TCP connected successfully!\n");
        return 0;
    }
    else
    {
        rt_kprintf("TCP connection failed! Error code: %d\n", ret);
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_tcp_connect, Connect to TCP server);

static int esp8266_tcp_disconnect(void)
{
    rt_kprintf("Disconnecting TCP...\n");

    int16_t ret = WIFI_DISCONNECT_TCP(&g_esp8266_wifi.base);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("TCP disconnected!\n");
        return 0;
    }
    else
    {
        rt_kprintf("TCP disconnect failed! Error code: %d\n", ret);
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_tcp_disconnect, Disconnect TCP connection);

static int esp8266_wifi_test(void)
{
    rt_kprintf("\n========== ESP8266 WiFi Test ==========\n");

    rt_kprintf("\n[1/4] Testing AT command...\n");
    if (esp8266_at_test() != 0)
    {
        rt_kprintf("\nTest failed at step 1!\n");
        return -1;
    }

    rt_thread_mdelay(100);

    rt_kprintf("\n[2/4] Setting Station mode...\n");
    if (esp8266_set_sta(0, RT_NULL) != 0)
    {
        rt_kprintf("\nTest failed at step 2!\n");
        return -1;
    }

    rt_thread_mdelay(100);

    rt_kprintf("\n[3/4] Connecting to WiFi: %s\n", ESP8266_WIFI_SSID);
    int16_t ret = WIFI_JOIN_AP(&g_esp8266_wifi.base, ESP8266_WIFI_SSID, ESP8266_WIFI_PASSWORD);
    if (ret != PLATFORM_WIFI_OK)
    {
        rt_kprintf("\nTest failed at step 3! Error: %d\n", ret);
        return -1;
    }

    rt_thread_mdelay(100);

    rt_kprintf("\n[4/4] Getting IP address...\n");
    if (esp8266_get_ip() != 0)
    {
        rt_kprintf("\nTest failed at step 4!\n");
        return -1;
    }

    rt_kprintf("\n========== Test Complete! ==========\n");
    rt_kprintf("All tests passed!\n");
    return 0;
}
MSH_CMD_EXPORT(esp8266_wifi_test, Run full WiFi test sequence);

static int esp8266_send_at(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: esp8266_send_at <AT_command>\n");
        rt_kprintf("Example: esp8266_send_at AT+GMR\n");
        return -1;
    }

    const char *cmd = argv[1];
    char resp[256] = {0};

    rt_kprintf("Sending: %s\n", cmd);

    int16_t ret = WIFI_SEND_AT_CMD(&g_esp8266_wifi.base, cmd, "OK", 2000);

    if (ret == PLATFORM_WIFI_OK)
    {
        rt_kprintf("Command executed successfully!\n");
        uint8_t *frame = wifi_esp8266_rx_get_frame(&g_esp8266_wifi);
        if (frame != RT_NULL)
        {
            rt_kprintf("Response:\n%s\n", (char *)frame);
        }
        return 0;
    }
    else
    {
        rt_kprintf("Command failed! Error code: %d\n", ret);
        return -1;
    }
}
MSH_CMD_EXPORT(esp8266_send_at, Send custom AT command);

static int mqtt_check_status(void)
{
    rt_kprintf("Checking MQTT connection status...\n");
    int16_t ret = MQTT_CHECK_CONNECTED(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID);
    if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("MQTT: Connected\n");
    else
        rt_kprintf("MQTT: Not connected (error=%d)\n", ret);
    return (ret == PLATFORM_MQTT_OK) ? 0 : -1;
}
MSH_CMD_EXPORT(mqtt_check_status, Check MQTT connection status);

static int mqtt_configure(int argc, char **argv)
{
    platform_mqtt_user_config_t config;

    memset(&config, 0, sizeof(config));

    if (argc >= 4)
    {
        strncpy(config.client_id, argv[1], sizeof(config.client_id) - 1);
        strncpy(config.username, argv[2], sizeof(config.username) - 1);
        strncpy(config.password, argv[3], sizeof(config.password) - 1);
    }
    else
    {
        strncpy(config.client_id, ONENET_DEVICE_NAME, sizeof(config.client_id) - 1);
        strncpy(config.username, ONENET_PRODUCT_ID, sizeof(config.username) - 1);
        strncpy(config.password, ONENET_MQTT_TOKEN, sizeof(config.password) - 1);
        rt_kprintf("Using default config:\n");
    }

    rt_kprintf("MQTT config: client=%s, user=%s\n", config.client_id, config.username);

    int16_t ret = MQTT_USERCFG(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID, &config);
    if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("MQTT user config OK!\n");
    else
        rt_kprintf("MQTT user config FAILED! (error=%d)\n", ret);

    return (ret == PLATFORM_MQTT_OK) ? 0 : -1;
}
MSH_CMD_EXPORT(mqtt_configure, Configure MQTT user[client_id username password] or use defaults);

static int mqtt_connect(int argc, char **argv)
{
    char host[64];
    uint16_t port = ONENET_MQTT_PORT;

    strncpy(host, ONENET_MQTT_HOST, sizeof(host) - 1);

    if (argc >= 2)
        strncpy(host, argv[1], sizeof(host) - 1);
    if (argc >= 3)
        port = (uint16_t)atoi(argv[2]);

    int16_t ret = MQTT_CHECK_CONNECTED(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID);
    if (ret == PLATFORM_MQTT_OK)
    {
        rt_kprintf("MQTT: Already connected!\n");
        return 0;
    }

    rt_kprintf("Connecting to MQTT server %s:%u...\n", host, port);

    ret = MQTT_CONNECT(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID, host, port, 1);
    if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("MQTT connect OK!\n");
    else
        rt_kprintf("MQTT connect FAILED! (error=%d)\n", ret);

    return (ret == PLATFORM_MQTT_OK) ? 0 : -1;
}
MSH_CMD_EXPORT(mqtt_connect, Connect to MQTT server[host][port]);

static int mqtt_subscribe(void)
{
    int16_t ret;
    int success_count = 0;
    int total_count = 0;

    rt_kprintf("Subscribing to all OneNET topics...\n");

#define SUBSCRIBE_TOPIC(topic_macro)                                                      \
    do                                                                                    \
    {                                                                                     \
        total_count++;                                                                    \
        rt_kprintf("  [%d] %s\n", total_count, topic_macro);                              \
        ret = MQTT_SUBSCRIBE(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID, topic_macro, 1); \
        if (ret == PLATFORM_MQTT_OK)                                                      \
        {                                                                                 \
            rt_kprintf("      OK\n");                                                     \
            success_count++;                                                              \
        }                                                                                 \
        else                                                                              \
        {                                                                                 \
            rt_kprintf("      FAILED! (error=%d)\n", ret);                                \
        }                                                                                 \
    } while (0)

    SUBSCRIBE_TOPIC(ONENET_TOPIC_PROPERTY_POST_REPLY);
    SUBSCRIBE_TOPIC(ONENET_TOPIC_PROPERTY_SET);
    SUBSCRIBE_TOPIC(ONENET_TOPIC_PROPERTY_GET);
    SUBSCRIBE_TOPIC(ONENET_TOPIC_PROPERTY_DESIRED_GET);
    SUBSCRIBE_TOPIC(ONENET_TOPIC_PROPERTY_DESIRED_DELETE);
    SUBSCRIBE_TOPIC(ONENET_TOPIC_OTA_INFORM);

#undef SUBSCRIBE_TOPIC

    rt_kprintf("\nSubscribe done! %d/%d topics subscribed successfully.\n", success_count, total_count);
    return (success_count == total_count) ? 0 : -1;
}
MSH_CMD_EXPORT(mqtt_subscribe, Subscribe to OneNET property topics);

static int mqtt_publish(int argc, char **argv)
{
    platform_mqtt_property_t prop;
    char msg_id[32] = "007";
    char value_buf[64];

    memset(&prop, 0, sizeof(prop));

    if (argc < 3)
    {
        rt_kprintf("Usage: mqtt_publish <key> <value> [type] [msg_id]\n");
        rt_kprintf("  type: 0=int 1=float 2=bool 3=string (default=0)\n");
        rt_kprintf("  Example: mqtt_publish BSP_LED 1 2\n");
        rt_kprintf("  Example: mqtt_publish temp 25.5 1\n");
        return -1;
    }

    strncpy(prop.key, argv[1], sizeof(prop.key) - 1);
    strncpy(value_buf, argv[2], sizeof(value_buf) - 1);

    if (argc >= 4)
        prop.value_type = (uint8_t)atoi(argv[3]);
    else
        prop.value_type = PLATFORM_MQTT_VALUE_INT;

    if (argc >= 5)
        strncpy(msg_id, argv[4], sizeof(msg_id) - 1);

    if (prop.value_type == PLATFORM_MQTT_VALUE_FLOAT)
        prop.value_float = atof(value_buf);
    else if (prop.value_type == PLATFORM_MQTT_VALUE_BOOL)
        prop.value_int = (strcmp(value_buf, "true") == 0 || strcmp(value_buf, "1") == 0) ? 1 : 0;
    else if (prop.value_type == PLATFORM_MQTT_VALUE_STRING)
        strncpy(prop.id, value_buf, sizeof(prop.id) - 1);
    else
        prop.value_int = atoi(value_buf);

    rt_kprintf("Publishing: %s = %s (type=%d, id=%s)\n", prop.key, value_buf, prop.value_type, msg_id);

    int16_t ret = MQTT_PUBLISH_PROPERTY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                        ONENET_PRODUCT_ID, ONENET_DEVICE_NAME, &prop, 1, msg_id);
    if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("Publish OK!\n");
    else
        rt_kprintf("Publish FAILED! (error=%d)\n", ret);

    return (ret == PLATFORM_MQTT_OK) ? 0 : -1;
}
MSH_CMD_EXPORT(mqtt_publish, Publish property<key><value>[type][msg_id]);

static int mqtt_listen(int argc, char **argv)
{
    uint32_t timeout_sec = 15;
    static char recv_topic[PLATFORM_MQTT_MAX_TOPIC_LEN];
    static char recv_payload[PLATFORM_MQTT_MAX_PAYLOAD_LEN];
    static char recv_msg_id[32];

    if (argc >= 2)
        timeout_sec = (uint32_t)atoi(argv[1]);

    rt_kprintf("Listening for MQTT messages for %u seconds...\n", (unsigned int)timeout_sec);
    rt_kprintf("  Filtering: property/set, property/get, ota/inform\n");
    rt_kprintf("  Ignoring:  property/post/reply\n");

    uint32_t start_tick = rt_tick_get();
    uint32_t timeout_ticks = timeout_sec * RT_TICK_PER_SECOND;
    uint32_t recv_count = 0;

    while ((rt_tick_get() - start_tick) < timeout_ticks)
    {
        memset(recv_topic, 0, sizeof(recv_topic));
        memset(recv_payload, 0, sizeof(recv_payload));
        memset(recv_msg_id, 0, sizeof(recv_msg_id));

        int16_t recv_ret = MQTT_CHECK_PROPERTY_SET_RECV(&g_esp8266_mqtt.base,
                                                        recv_topic, recv_payload,
                                                        sizeof(recv_payload), recv_msg_id);
        if (recv_ret == PLATFORM_MQTT_OK)
        {
            if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_POST_REPLY) == 0)
            {
                wifi_esp8266_rx_restart(g_esp8266_mqtt.wifi);
                continue;
            }

            recv_count++;
            rt_kprintf("\n[%u] Received: topic=%s\n", recv_count, recv_topic);
            rt_kprintf("  Payload: %s\n", recv_payload);
            rt_kprintf("  Message ID: %s\n", recv_msg_id);

            if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_SET) == 0)
            {
                rt_kprintf("  Auto replying to property set...\n");
                if (MQTT_PUBLISH_SET_REPLY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                           ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                           recv_msg_id, 200, "user_succ") == PLATFORM_MQTT_OK)
                    rt_kprintf("  Reply sent!\n");
                else
                    rt_kprintf("  Reply FAILED!\n");
            }
            else if (strcmp(recv_topic, ONENET_TOPIC_PROPERTY_GET) == 0)
            {
                rt_kprintf("  Property get request received (manual reply needed)\n");
            }
            else if (strcmp(recv_topic, ONENET_TOPIC_OTA_INFORM) == 0)
            {
                rt_kprintf("  OTA inform received\n");
            }

            wifi_esp8266_rx_restart(g_esp8266_mqtt.wifi);
        }

        rt_thread_mdelay(10);
    }

    rt_kprintf("\nListen finished. Received %u messages.\n", (unsigned int)recv_count);
    return 0;
}
MSH_CMD_EXPORT(mqtt_listen, Listen for MQTT messages [timeout_sec]);

static int mqtt_disconnect(void)
{
    rt_kprintf("Disconnecting MQTT...\n");
    int16_t ret = MQTT_DISCONNECT(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID);
    if (ret == PLATFORM_MQTT_OK)
        rt_kprintf("MQTT disconnected!\n");
    else
        rt_kprintf("Disconnect FAILED! (error=%d)\n", ret);
    return (ret == PLATFORM_MQTT_OK) ? 0 : -1;
}
MSH_CMD_EXPORT(mqtt_disconnect, Disconnect from MQTT broker);

static int mqtt_full_test(int argc, char **argv)
{
    rt_kprintf("\n========== MQTT Full Test ==========\n");

    rt_kprintf("\n[1/6] Configuring MQTT user...\n");
    if (mqtt_configure(0, RT_NULL) != 0)
    {
        rt_kprintf("Configure FAILED!\n");
        return -1;
    }
    rt_thread_mdelay(200);

    rt_kprintf("\n[2/6] Connecting to MQTT server...\n");
    if (mqtt_connect(0, RT_NULL) != 0)
    {
        rt_kprintf("Connect FAILED!\n");
        return -1;
    }
    rt_thread_mdelay(200);

    rt_kprintf("\n[3/6] Subscribing to topics...\n");
    mqtt_subscribe();
    rt_thread_mdelay(200);

    rt_kprintf("\n[4/6] Publishing properties...\n");
    {
        platform_mqtt_property_t props[3];
        dht11_data_t dht11_data;
        int prop_count = 0;
        int16_t ret;

        memset(props, 0, sizeof(props));

        strncpy(props[0].key, "BSP_LED", sizeof(props[0].key) - 1);
        props[0].value_int = 1;
        props[0].value_type = PLATFORM_MQTT_VALUE_BOOL;
        prop_count = 1;

        if (dht11_read(&dht11_data) == DHT11_OK)
        {
            strncpy(props[1].key, "TEMPERATURE", sizeof(props[1].key) - 1);
            props[1].value_int = dht11_data.temperature_int;
            props[1].value_type = PLATFORM_MQTT_VALUE_INT;

            strncpy(props[2].key, "HUMIDITY", sizeof(props[2].key) - 1);
            props[2].value_int = dht11_data.humidity_int;
            props[2].value_type = PLATFORM_MQTT_VALUE_INT;

            prop_count = 3;
            rt_kprintf("  DHT11: Temp=%dC, Humi=%d%%\n",
                       dht11_data.temperature_int, dht11_data.humidity_int);
        }
        else
        {
            rt_kprintf("  DHT11 read failed, publishing BSP_LED only\n");
        }

        ret = MQTT_PUBLISH_PROPERTY(&g_esp8266_mqtt.base, ESP8266_MQTT_LINK_ID,
                                    ONENET_PRODUCT_ID, ONENET_DEVICE_NAME,
                                    props, prop_count, "001");
        if (ret == PLATFORM_MQTT_OK)
            rt_kprintf("Publish OK! (%d properties)\n", prop_count);
        else
            rt_kprintf("Publish FAILED! (error=%d)\n", ret);
    }
    rt_thread_mdelay(200);

    rt_kprintf("\n[5/6] Listening for 15 seconds...\n");
    mqtt_listen(0, RT_NULL);
    rt_thread_mdelay(200);

    rt_kprintf("\n[6/6] Disconnecting...\n");
    mqtt_disconnect();

    rt_kprintf("\n========== MQTT Test Complete! ==========\n");
    return 0;
}
MSH_CMD_EXPORT(mqtt_full_test, Run full MQTT test sequence);

extern onenet_kv_table_t g_kv_table;

static int kv_list(void)
{
    rt_kprintf("OneNet KV Table (%d entries):\n", g_kv_table.count);
    for (uint8_t i = 0; i < g_kv_table.count; i++)
    {
        onenet_kv_entry_t *e = &g_kv_table.entries[i];
        rt_kprintf("  [%d] key=%-12s type=%d dirty=%d", i, e->key, e->value_type, e->dirty);
        switch (e->value_type)
        {
        case PLATFORM_MQTT_VALUE_INT:
            rt_kprintf(" value=%d\n", *((int *)e->value_ptr));
            break;
        case PLATFORM_MQTT_VALUE_FLOAT:
            rt_kprintf(" value=%.2f\n", *((float *)e->value_ptr));
            break;
        case PLATFORM_MQTT_VALUE_BOOL:
            rt_kprintf(" value=%s\n", *((uint8_t *)e->value_ptr) ? "true" : "false");
            break;
        case PLATFORM_MQTT_VALUE_STRING:
            rt_kprintf(" value=%s\n", (char *)e->value_ptr);
            break;
        default:
            rt_kprintf(" value=?\n");
            break;
        }
    }
    return 0;
}
MSH_CMD_EXPORT(kv_list, List all OneNet KV entries);

static int kv_set(int argc, char **argv)
{
    if (argc < 3)
    {
        rt_kprintf("Usage: kv_set <key> <value>\n");
        rt_kprintf("  Example: kv_set BSP_LED 1\n");
        rt_kprintf("  Example: kv_set BSP_LED 0\n");
        return -1;
    }

    int8_t ret = onenet_kv_set_value(&g_kv_table, argv[1], argv[2]);
    if (ret == 0)
        rt_kprintf("Set %s = %s OK\n", argv[1], argv[2]);
    else
        rt_kprintf("Set FAILED! (error=%d, key not found?)\n", ret);
    return (ret == 0) ? 0 : -1;
}
MSH_CMD_EXPORT(kv_set, Set OneNet KV value<key><value>);

#endif
