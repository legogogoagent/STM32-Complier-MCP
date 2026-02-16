/**
 * @file swd_driver.h
 * @brief SWD (Serial Wire Debug) Protocol Driver
 * 
 * @details Implements SWD protocol using GPIO bit-banging.
 *          Supports STM32F1/F4 series microcontrollers.
 * 
 * @author ESP32-STM32 Bridge Project
 * @date 2026-02-15
 */

#ifndef SWD_DRIVER_H
#define SWD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// SWD Constants
#define SWD_OK              0
#define SWD_ERROR           1
#define SWD_WAIT           2
#define SWD_FAULT          3

// SWD Register Addresses (AP = 1, DP = 0)
#define SWD_DP_IDCODE       0x00
#define SWD_DP_STATUS       0x04
#define SWD_DP_CONTROL      0x04
#define SWD_DP_WCR          0x04
#define SWD_DP_RESEND       0x08
#define SWD_DP_SELECT       0x08
#define SWD_DP_RDBUFF       0x0C

#define SWD_AP_CSW          0x00
#define SWD_AP_TAR          0x04
#define SWD_AP_DRW          0x0C
#define SWD_AP_CFG          0x0F
#define SWD_AP_BASE         0x0F
#define SWD_AP_IDR          0x0F

// SWD ACK Values
#define SWD_ACK_OK          0x01
#define SWD_ACK_WAIT        0x02
#define SWD_ACK_FAULT       0x04

// JTAG-to-SWD Sequence
#define SWD_JTAG_TO_SWD     0xE79E

// SWD Clock Delay (adjust for speed)
// Lower = faster, Higher = more reliable
#define SWD_CLK_DELAY_US    2

/**
 * @brief SWD Driver Handle
 */
typedef struct {
    gpio_num_t swdio_pin;      // SWD Data pin
    gpio_num_t swclk_pin;      // SWD Clock pin
    gpio_num_t nrst_pin;       // Target reset pin (optional)
    uint32_t clock_delay_us;   // Clock delay in microseconds
    bool initialized;
} swd_driver_t;

/**
 * @brief SWD IDCODE structure
 */
typedef struct {
    uint32_t idcode;           // Device IDCODE
    uint16_t part_number;      // Part number (bits 0-11)
    uint8_t  manufacturer;      // Manufacturer JEDEC ID (bits 12-17)
    uint8_t  revision;         // Revision number (bits 28-31)
} swd_idcode_t;

/**
 * @brief Initialize SWD driver
 * @param handle SWD driver handle
 * @param swdio GPIO pin for SWDIO
 * @param swclk GPIO pin for SWCLK
 * @param nrst GPIO pin for NRST (0 to disable)
 * @return ESP_OK on success
 */
esp_err_t swd_init(swd_driver_t *handle, gpio_num_t swdio, gpio_num_t swclk, gpio_num_t nrst);

/**
 * @brief Deinitialize SWD driver
 * @param handle SWD driver handle
 * @return ESP_OK on success
 */
esp_err_t swd_deinit(swd_driver_t *handle);

/**
 * @brief Perform SWD line reset (50+ clock cycles with SWDIO high)
 * @param handle SWD driver handle
 * @return ESP_OK on success
 */
esp_err_t swd_line_reset(swd_driver_t *handle);

/**
 * @brief Send JTAG-to-SWD sequence
 * @param handle SWD driver handle
 * @return ESP_OK on success
 */
esp_err_t swd_jtag_to_swd(swd_driver_t *handle);

/**
 * @brief Perform full SWD reset sequence
 * @param handle SWD driver handle
 * @return ESP_OK on success
 */
esp_err_t swd_reset(swd_driver_t *handle);

/**
 * @brief Read target IDCODE
 * @param handle SWD driver handle
 * @param idcode Pointer to store IDCODE
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_read_idcode(swd_driver_t *handle, uint32_t *idcode);

/**
 * @brief Read DP register
 * @param handle SWD driver handle
 * @param addr Register address (0x0-0xC)
 * @param data Pointer to store data
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_read_dp(swd_driver_t *handle, uint8_t addr, uint32_t *data);

/**
 * @brief Write DP register
 * @param handle SWD driver handle
 * @param addr Register address (0x0-0xC)
 * @param data Data to write
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_write_dp(swd_driver_t *handle, uint8_t addr, uint32_t data);

/**
 * @brief Read AP register
 * @param handle SWD driver handle
 * @param addr Register address (0x0-0xF)
 * @param data Pointer to store data
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_read_ap(swd_driver_t *handle, uint8_t addr, uint32_t *data);

/**
 * @brief Write AP register
 * @param handle SWD driver handle
 * @param addr Register address (0x0-0xF)
 * @param data Data to write
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_write_ap(swd_driver_t *handle, uint8_t addr, uint32_t data);

/**
 * @brief Read memory via AP
 * @param handle SWD driver handle
 * @param address Memory address
 * @param data Pointer to store data
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_read_mem(swd_driver_t *handle, uint32_t address, uint32_t *data);

/**
 * @brief Write memory via AP
 * @param handle SWD driver handle
 * @param address Memory address
 * @param data Data to write
 * @return SWD_OK on success, error code otherwise
 */
uint8_t swd_write_mem(swd_driver_t *handle, uint32_t address, uint32_t data);

/**
 * @brief Reset target MCU via NRST pin
 * @param handle SWD driver handle
 * @return ESP_OK on success
 */
esp_err_t swd_hw_reset(swd_driver_t *handle);

/**
 * @brief Detect STM32 MCU type from IDCODE
 * @param idcode Device IDCODE
 * @param name Buffer to store MCU name
 * @param name_len Buffer length
 * @return ESP_OK on success
 */
esp_err_t swd_detect_mcu(uint32_t idcode, char *name, size_t name_len);

#ifdef __cplusplus
}
#endif

#endif // SWD_DRIVER_H
