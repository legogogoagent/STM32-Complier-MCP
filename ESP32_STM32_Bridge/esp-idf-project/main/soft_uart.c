#include "soft_uart.h"
#include "bridge_config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "SOFT_UART";

static soft_uart_handle_t *s_handle = NULL;
static QueueHandle_t s_gpio_queue = NULL;

static void gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(s_gpio_queue, &gpio_num, NULL);
}

static void delay_us(uint32_t us)
{
    for (uint32_t i = 0; i < us; i++) {
        __asm__ __volatile__("nop");
    }
}

static void uart_rx_task(void *pvParameters)
{
    soft_uart_handle_t *handle = (soft_uart_handle_t *)pvParameters;
    uint32_t gpio_num;
    uint8_t received_byte = 0;
    int bit_count = 0;
    
    uint32_t bit_delay_us = 1000000 / handle->baudrate;
    uint32_t half_bit = bit_delay_us / 2;

    while (1) {
        if (xQueueReceive(s_gpio_queue, &gpio_num, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (gpio_get_level(handle->rx_gpio) == 0) {
                delay_us(half_bit);
                
                received_byte = 0;
                bit_count = 0;
                
                for (int i = 0; i < 8; i++) {
                    received_byte >>= 1;
                    if (gpio_get_level(handle->rx_gpio)) {
                        received_byte |= 0x80;
                    }
                    delay_us(bit_delay_us);
                }
                
                delay_us(bit_delay_us);
                
                uint16_t next_head = (handle->ring_head + 1) % SOFT_UART_RING_SIZE;
                if (next_head != handle->ring_tail) {
                    handle->ring_buffer[handle->ring_head] = received_byte;
                    handle->ring_head = next_head;
                    gpio_set_level(GPIO_SYSTEM_LED, 1);
                    vTaskDelay(50);
                    gpio_set_level(GPIO_SYSTEM_LED, 0);
                    ESP_LOGI(TAG, "RX: got byte 0x%02X", received_byte);
                }
            }
        }
    }
}

esp_err_t soft_uart_init(soft_uart_handle_t *handle, int tx_gpio, int rx_gpio, uint32_t baudrate)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (baudrate < SOFT_UART_MIN_BAUD || baudrate > SOFT_UART_MAX_BAUD) {
        ESP_LOGW(TAG, "Baudrate %lu out of range, using 115200", baudrate);
        baudrate = 115200;
    }

    handle->tx_gpio = tx_gpio;
    handle->rx_gpio = rx_gpio;
    handle->baudrate = baudrate;
    handle->state = SOFT_UART_STATE_STOP;
    handle->ring_head = 0;
    handle->ring_tail = 0;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << tx_gpio) | (1ULL << rx_gpio),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(tx_gpio, 1);

    s_handle = handle;

    s_gpio_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(rx_gpio, gpio_isr_handler, (void *)rx_gpio);

    ESP_LOGI(TAG, "Soft UART initialized: TX=GPIO%d, RX=GPIO%d, baud=%lu", tx_gpio, rx_gpio, baudrate);
    return ESP_OK;
}

esp_err_t soft_uart_start(soft_uart_handle_t *handle)
{
    if (!handle || handle->state == SOFT_UART_STATE_RUNNING) {
        return ESP_ERR_INVALID_STATE;
    }

    s_handle = handle;
    handle->state = SOFT_UART_STATE_RUNNING;
    
    xTaskCreate(uart_rx_task, "uart_rx", 2048, handle, 10, NULL);
    
    ESP_LOGI(TAG, "Soft UART started at %lu baud", handle->baudrate);
    return ESP_OK;
}

esp_err_t soft_uart_stop(soft_uart_handle_t *handle)
{
    if (!handle || handle->state == SOFT_UART_STATE_STOP) {
        return ESP_ERR_INVALID_STATE;
    }

    handle->ring_head = 0;
    handle->ring_tail = 0;
    handle->state = SOFT_UART_STATE_STOP;

    ESP_LOGI(TAG, "Soft UART stopped");
    return ESP_OK;
}

esp_err_t soft_uart_set_baudrate(soft_uart_handle_t *handle, uint32_t baudrate)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (baudrate < SOFT_UART_MIN_BAUD || baudrate > SOFT_UART_MAX_BAUD) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->baudrate = baudrate;
    ESP_LOGI(TAG, "Baudrate changed to %lu", baudrate);

    return ESP_OK;
}

esp_err_t soft_uart_write(soft_uart_handle_t *handle, const uint8_t *data, size_t len)
{
    if (!handle || !data || len == 0 || handle->state != SOFT_UART_STATE_RUNNING) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "TX: sending %d bytes at baud %lu", len, handle->baudrate);
    
    gpio_set_level(GPIO_WIFI_LED, 1);
    
    uint32_t bit_delay_us = 1000000 / handle->baudrate;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        
        gpio_set_level(handle->tx_gpio, 0);
        delay_us(bit_delay_us);
        
        for (int bit = 0; bit < 8; bit++) {
            gpio_set_level(handle->tx_gpio, (byte >> bit) & 1);
            delay_us(bit_delay_us);
        }
        
        gpio_set_level(handle->tx_gpio, 1);
        delay_us(bit_delay_us);
    }

    gpio_set_level(GPIO_WIFI_LED, 0);
    ESP_LOGI(TAG, "TX: done");
    return ESP_OK;
}

int soft_uart_read(soft_uart_handle_t *handle, uint8_t *data, size_t max_len)
{
    if (!handle || !data || max_len == 0) {
        return -1;
    }

    uint16_t head = handle->ring_head;
    uint16_t tail = handle->ring_tail;
    int count = 0;

    while (head != tail && count < max_len) {
        data[count++] = handle->ring_buffer[tail];
        tail = (tail + 1) % SOFT_UART_RING_SIZE;
    }
    handle->ring_tail = tail;

    return count;
}

bool soft_uart_has_data(soft_uart_handle_t *handle)
{
    return handle && handle->ring_head != handle->ring_tail;
}

uint32_t soft_uart_get_baudrate(soft_uart_handle_t *handle)
{
    return handle ? handle->baudrate : 0;
}

soft_uart_state_t soft_uart_get_state(soft_uart_handle_t *handle)
{
    return handle ? handle->state : SOFT_UART_STATE_STOP;
}
