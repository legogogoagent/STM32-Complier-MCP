#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "bridge_config.h"
#include "soft_uart.h"
#include "mcp_server.h"
#include "session_manager.h"

static const char *TAG = "MCP";

soft_uart_handle_t s_uart_handle;
bool s_uart_initialized = false;

static char *mcp_uart_start(int argc, char **argv)
{
    if (argc < 2) {
        return "ERROR: Usage: uart_start <baudrate>";
    }

    uint32_t baudrate = atoi(argv[1]);
    if (baudrate < 9600 || baudrate > 115200) {
        return "ERROR: Baudrate must be between 9600 and 115200";
    }

    if (!s_uart_initialized) {
        esp_err_t err = soft_uart_init(&s_uart_handle, GPIO_UART_TX, GPIO_UART_RX, baudrate);
        if (err != ESP_OK) {
            return "ERROR: Failed to initialize UART";
        }
        s_uart_initialized = true;
    }

    soft_uart_set_baudrate(&s_uart_handle, baudrate);
    soft_uart_start(&s_uart_handle);

    static char response[64];
    snprintf(response, sizeof(response), "OK: Started at %lu", baudrate);
    return response;
}

static char *mcp_uart_stop(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!s_uart_initialized) {
        return "ERROR: UART not initialized";
    }

    soft_uart_stop(&s_uart_handle);
    return "OK: Stopped";
}

static char *mcp_uart_send(int argc, char **argv)
{
    if (argc < 2) {
        return "ERROR: Usage: uart_send <hex_data>";
    }

    if (!s_uart_initialized || soft_uart_get_state(&s_uart_handle) != SOFT_UART_STATE_RUNNING) {
        return "ERROR: UART not running";
    }

    char *hex_str = argv[1];
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0) {
        return "ERROR: Hex data must have even length";
    }

    uint8_t data[256];
    size_t data_len = hex_len / 2;

    for (size_t i = 0; i < data_len; i++) {
        char byte_str[3] = {hex_str[i*2], hex_str[i*2+1], 0};
        char *endptr;
        data[i] = strtol(byte_str, &endptr, 16);
        if (*endptr != 0) {
            return "ERROR: Invalid hex data";
        }
    }

    soft_uart_write(&s_uart_handle, data, data_len);

    static char response[64];
    snprintf(response, sizeof(response), "OK: Sent %zu bytes", data_len);
    return response;
}

static char *mcp_uart_recv(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!s_uart_initialized || soft_uart_get_state(&s_uart_handle) != SOFT_UART_STATE_RUNNING) {
        return "ERROR: UART not running";
    }

    uint8_t data[256];
    int len = soft_uart_read(&s_uart_handle, data, sizeof(data));

    if (len <= 0) {
        return "OK: ";
    }

    static char response[512];
    char *ptr = response;
    ptr += snprintf(ptr, 64, "OK: ");
    for (int i = 0; i < len; i++) {
        ptr += snprintf(ptr, 8, "%02X", data[i]);
    }
    return response;
}

typedef struct {
    const char *cmd;
    char *(*handler)(int argc, char **argv);
    const char *desc;
} mcp_command_t;

static mcp_command_t s_commands[] = {
    {"uart_start", mcp_uart_start, "uart_start <baud> - Start UART bridge"},
    {"uart_stop", mcp_uart_stop, "uart_stop - Stop UART bridge"},
    {"uart_send", mcp_uart_send, "uart_send <hex> - Send hex data"},
    {"uart_recv", mcp_uart_recv, "uart_recv - Receive data"},
    {NULL, NULL, NULL},
};

static char *mcp_process_command(char *cmd_line)
{
    char *argv[16];
    int argc = 0;
    char *token = strtok(cmd_line, " \t\n");
    while (token && argc < 16) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (argc == 0) {
        return "ERROR: Empty command";
    }

    for (mcp_command_t *cmd = s_commands; cmd->cmd; cmd++) {
        if (strcmp(argv[0], cmd->cmd) == 0) {
            return cmd->handler(argc, argv);
        }
    }

    return "ERROR: Unknown command";
}

static void mcp_handle_client(int sock)
{
    char recv_buf[512];
    char send_buf[512];

    snprintf(send_buf, sizeof(send_buf), "ESP32-STM32-Bridge v1.1\r\n");
    send(sock, send_buf, strlen(send_buf), 0);

    while (1) {
        int len = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
        if (len <= 0) {
            break;
        }
        recv_buf[len] = 0;

        char *line = strtok(recv_buf, "\r\n");
        while (line) {
            ESP_LOGI(TAG, "Received: %s", line);
            char *response = mcp_process_command(line);
            snprintf(send_buf, sizeof(send_buf), "%s\r\n", response);
            send(sock, send_buf, strlen(send_buf), 0);
            line = strtok(NULL, "\r\n");
        }
    }
    close(sock);
}

static void mcp_server_task(void *pvParameters) {
    (void)pvParameters;
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(4444);
    ip_protocol = IPPROTO_IP;

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    listen(listen_sock, 1);

    ESP_LOGI(TAG, "MCP Server listening on port 4444");

    while (1) {
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Client connected: %s", addr_str);

        mcp_handle_client(sock);

        ESP_LOGI(TAG, "Client disconnected: %s", addr_str);
    }

    vTaskDelete(NULL);
}

void mcp_server_init(void) {
    xTaskCreate(mcp_server_task, "mcp_server", 4096, NULL, 5, NULL);
}
