/**
 * @file swd_driver.c
 * @brief SWD Protocol Implementation
 */

#include "swd_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "bridge_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SWD";

#define SWD_CLOCK()   \
    do {             \
        gpio_set_level(handle->swclk_pin, 1);  \
        vTaskDelay(1); \
        gpio_set_level(handle->swclk_pin, 0);  \
        vTaskDelay(1); \
    } while(0)

static void swd_write_bit(swd_driver_t *handle, uint8_t bit)
{
    gpio_set_level(handle->swdio_pin, bit ? 1 : 0);
    SWD_CLOCK();
}

static uint8_t swd_read_bit(swd_driver_t *handle)
{
    uint8_t bit = gpio_get_level(handle->swdio_pin);
    SWD_CLOCK();
    return bit;
}

static void swd_write_byte(swd_driver_t *handle, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        swd_write_bit(handle, (byte >> i) & 1);
    }
}

static uint8_t swd_read_byte(swd_driver_t *handle)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (swd_read_bit(handle) << i);
    }
    return byte;
}

static uint8_t swd_parity(uint32_t data)
{
    uint8_t parity = 0;
    while (data) {
        parity ^= (data & 1);
        data >>= 1;
    }
    return parity;
}

esp_err_t swd_init(swd_driver_t *handle, gpio_num_t swdio, gpio_num_t swclk, gpio_num_t nrst)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->swdio_pin = swdio;
    handle->swclk_pin = swclk;
    handle->nrst_pin = nrst;
    handle->clock_delay_us = SWD_CLK_DELAY_US;
    handle->initialized = false;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << swdio) | (1ULL << swclk),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(handle->swclk_pin, 0);
    gpio_set_level(handle->swdio_pin, 0);

    if (nrst > 0) {
        gpio_config_t nrst_conf = {
            .pin_bit_mask = (1ULL << nrst),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&nrst_conf);
        gpio_set_level(nrst, 1);
    }

    handle->initialized = true;
    ESP_LOGI(TAG, "SWD initialized on GPIO%d/%d", swdio, swclk);

    return ESP_OK;
}

esp_err_t swd_deinit(swd_driver_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    handle->initialized = false;
    return ESP_OK;
}

esp_err_t swd_line_reset(swd_driver_t *handle)
{
    gpio_set_direction(handle->swdio_pin, GPIO_MODE_OUTPUT);
    
    gpio_set_level(handle->swdio_pin, 1);
    for (int i = 0; i < 50; i++) {
        SWD_CLOCK();
    }
    
    return ESP_OK;
}

esp_err_t swd_jtag_to_swd(swd_driver_t *handle)
{
    uint16_t seq = SWD_JTAG_TO_SWD;
    
    for (int i = 0; i < 16; i++) {
        swd_write_bit(handle, (seq >> i) & 1);
    }
    
    for (int i = 0; i < 8; i++) {
        swd_write_bit(handle, 0);
    }
    
    return ESP_OK;
}

esp_err_t swd_reset(swd_driver_t *handle)
{
    swd_line_reset(handle);
    swd_jtag_to_swd(handle);
    
    for (int i = 0; i < 8; i++) {
        swd_write_bit(handle, 0);
    }
    
    return ESP_OK;
}

static uint8_t swd_transfer(swd_driver_t *handle, uint8_t request, uint32_t *data)
{
    uint8_t ack = 0;
    uint32_t read_data = 0;
    uint8_t rnw = (request >> 1) & 1;
    
    gpio_set_direction(handle->swdio_pin, GPIO_MODE_OUTPUT);
    
    for (int i = 0; i < 8; i++) {
        swd_write_bit(handle, (request >> i) & 1);
    }
    
    swd_parity(request);
    
    gpio_set_direction(handle->swdio_pin, GPIO_MODE_INPUT);
    
    for (int i = 0; i < 3; i++) {
        ack |= (swd_read_bit(handle) << i);
    }
    
    if (ack != SWD_ACK_OK) {
        gpio_set_direction(handle->swdio_pin, GPIO_MODE_OUTPUT);
        return ack;
    }
    
    if (rnw) {
        for (int i = 0; i < 32; i++) {
            read_data |= (swd_read_bit(handle) << i);
        }
        
        swd_read_bit(handle);
        
        if (data) {
            *data = read_data;
        }
    } else {
        gpio_set_direction(handle->swdio_pin, GPIO_MODE_OUTPUT);
        
        for (int i = 0; i < 32; i++) {
            swd_write_bit(handle, (*data >> i) & 1);
        }
        
        swd_write_bit(handle, swd_parity(*data));
    }
    
    gpio_set_direction(handle->swdio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(handle->swdio_pin, 0);
    SWD_CLOCK();
    
    return ack;
}

uint8_t swd_read_idcode(swd_driver_t *handle, uint32_t *idcode)
{
    if (!handle || !idcode) {
        return SWD_ERROR;
    }

    swd_reset(handle);
    
    uint8_t request = 0xA5;
    uint8_t ack = swd_transfer(handle, request, idcode);
    
    if (ack == SWD_ACK_OK) {
        ESP_LOGI(TAG, "IDCODE: 0x%08lx", *idcode);
        return SWD_OK;
    }
    
    ESP_LOGE(TAG, "Failed to read IDCODE, ACK: 0x%x", ack);
    return SWD_ERROR;
}

uint8_t swd_read_dp(swd_driver_t *handle, uint8_t addr, uint32_t *data)
{
    uint8_t request = 0x81 | ((addr & 0x0C) << 1);
    uint8_t ack = swd_transfer(handle, request, data);
    
    if (ack != SWD_ACK_OK) {
        return ack;
    }
    
    request = 0x83;
    return swd_transfer(handle, request, data);
}

uint8_t swd_write_dp(swd_driver_t *handle, uint8_t addr, uint32_t data)
{
    uint8_t request = 0x81 | ((addr & 0x0C) << 1);
    uint8_t ack = swd_transfer(handle, request, &data);
    
    if (ack != SWD_ACK_OK) {
        return ack;
    }
    
    request = 0x83;
    return swd_transfer(handle, request, &data);
}

uint8_t swd_read_ap(swd_driver_t *handle, uint8_t addr, uint32_t *data)
{
    uint8_t request = 0x85 | ((addr & 0x0C) << 1);
    uint8_t ack = swd_transfer(handle, request, data);
    
    if (ack != SWD_ACK_OK) {
        return ack;
    }
    
    request = 0x83;
    return swd_transfer(handle, request, data);
}

uint8_t swd_write_ap(swd_driver_t *handle, uint8_t addr, uint32_t data)
{
    uint8_t request = 0x85 | ((addr & 0x0C) << 1);
    return swd_transfer(handle, request, &data);
}

uint8_t swd_read_mem(swd_driver_t *handle, uint32_t address, uint32_t *data)
{
    if (swd_write_ap(handle, SWD_AP_TAR, address) != SWD_ACK_OK) {
        return SWD_ERROR;
    }
    
    return swd_read_ap(handle, SWD_AP_DRW, data);
}

uint8_t swd_write_mem(swd_driver_t *handle, uint32_t address, uint32_t data)
{
    if (swd_write_ap(handle, SWD_AP_TAR, address) != SWD_ACK_OK) {
        return SWD_ERROR;
    }
    
    return swd_write_ap(handle, SWD_AP_DRW, data);
}

esp_err_t swd_hw_reset(swd_driver_t *handle)
{
    if (!handle || handle->nrst_pin == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    gpio_set_level(handle->nrst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(handle->nrst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    return swd_reset(handle);
}

esp_err_t swd_detect_mcu(uint32_t idcode, char *name, size_t name_len)
{
    if (!name || name_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint16_t part_num = idcode & 0xFFF;
    uint8_t rev = (idcode >> 28) & 0xF;
    
    switch (part_num) {
        case 0x412:
            snprintf(name, name_len, "STM32F10x (F1 Medium)");
            break;
        case 0x413:
            snprintf(name, name_len, "STM32F10x (F1 High)");
            break;
        case 0x414:
            snprintf(name, name_len, "STM32F10x (F1 XL)");
            break;
        case 0x423:
            snprintf(name, name_len, "STM32F2xx (F2)");
            break;
        case 0x431:
            snprintf(name, name_len, "STM32F401x");
            break;
        case 0x433:
            snprintf(name, name_len, "STM32F4xx (F411)");
            break;
        case 0x437:
            snprintf(name, name_len, "STM32F4xx (F407/F417)");
            break;
        case 0x449:
            snprintf(name, name_len, "STM32F7xx");
            break;
        case 0x460:
            snprintf(name, name_len, "STM32H7xx");
            break;
        default:
            snprintf(name, name_len, "Unknown (0x%03X rev %d)", part_num, rev);
            break;
    }
    
    return ESP_OK;
}
