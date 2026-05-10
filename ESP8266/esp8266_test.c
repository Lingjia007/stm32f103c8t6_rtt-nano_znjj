#include <rtthread.h>
#include <stdlib.h>
#include <string.h>
#include "esp8266_init.h"
#include "esp8266_config.h"

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

#endif
