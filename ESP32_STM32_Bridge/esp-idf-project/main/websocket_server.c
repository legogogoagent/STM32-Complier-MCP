#include "websocket_server.h"
#include "soft_uart.h"
#include "bridge_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WS_SERVER";

static bool s_ws_running = false;

esp_err_t websocket_server_init(void)
{
    if (s_ws_running) {
        return ESP_OK;
    }

    s_ws_running = true;
    ESP_LOGI(TAG, "WebSocket server init (stub)");
    return ESP_OK;
}

esp_err_t websocket_server_stop(void)
{
    if (!s_ws_running) {
        return ESP_OK;
    }

    s_ws_running = false;
    ESP_LOGI(TAG, "WebSocket server stopped");
    return ESP_OK;
}

bool websocket_server_is_running(void)
{
    return s_ws_running;
}
