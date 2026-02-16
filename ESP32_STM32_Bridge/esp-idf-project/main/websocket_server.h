#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define WS_UART_PORT 8080

esp_err_t websocket_server_init(void);
esp_err_t websocket_server_stop(void);
bool websocket_server_is_running(void);
