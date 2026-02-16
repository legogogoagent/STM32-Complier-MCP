#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define SOFT_UART_MAX_BAUD     115200
#define SOFT_UART_MIN_BAUD     9600
#define SOFT_UART_RING_SIZE    1024

typedef enum {
    SOFT_UART_STATE_STOP,
    SOFT_UART_STATE_RUNNING,
} soft_uart_state_t;

typedef struct {
    int tx_gpio;
    int rx_gpio;
    uint32_t baudrate;
    soft_uart_state_t state;
    uint8_t ring_buffer[SOFT_UART_RING_SIZE];
    volatile uint16_t ring_head;
    volatile uint16_t ring_tail;
} soft_uart_handle_t;

esp_err_t soft_uart_init(soft_uart_handle_t *handle, int tx_gpio, int rx_gpio, uint32_t baudrate);
esp_err_t soft_uart_start(soft_uart_handle_t *handle);
esp_err_t soft_uart_stop(soft_uart_handle_t *handle);
esp_err_t soft_uart_set_baudrate(soft_uart_handle_t *handle, uint32_t baudrate);
esp_err_t soft_uart_write(soft_uart_handle_t *handle, const uint8_t *data, size_t len);
int soft_uart_read(soft_uart_handle_t *handle, uint8_t *data, size_t max_len);
bool soft_uart_has_data(soft_uart_handle_t *handle);
uint32_t soft_uart_get_baudrate(soft_uart_handle_t *handle);
soft_uart_state_t soft_uart_get_state(soft_uart_handle_t *handle);
