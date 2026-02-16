#ifndef FLASH_WRITER_H
#define FLASH_WRITER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "swd_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_BASE_ADDR           0x08000000
#define STM32F1_FLASH_KEY1        0x45670123
#define STM32F1_FLASH_KEY2        0xCDEF89AB

#define FLASH_ACR                 0x40022000
#define FLASH_KEYR                0x40022004
#define FLASH_OPTKEYR             0x40022008
#define FLASH_SR                  0x4002200C
#define FLASH_CR                  0x40022010
#define FLASH_AR                  0x40022014
#define FLASH_OBR                 0x4002201C
#define FLASH_WRPR                0x40022020

#define FLASH_SR_BSY              (1 << 0)
#define FLASH_SR_PGERR            (1 << 2)
#define FLASH_SR_WRPRTERR         (1 << 4)
#define FLASH_SR_EOP              (1 << 5)

#define FLASH_CR_PG               (1 << 0)
#define FLASH_CR_PER              (1 << 1)
#define FLASH_CR_MER              (1 << 2)
#define FLASH_CR_STRT             (1 << 6)
#define FLASH_CR_LOCK             (1 << 7)

typedef struct {
    swd_driver_t *swd;
    bool unlocked;
    uint32_t flash_size;
    uint16_t page_size;
    uint8_t  device_id;
} flash_writer_t;

esp_err_t flash_init(flash_writer_t *fw, swd_driver_t *swd);

esp_err_t flash_deinit(flash_writer_t *fw);

esp_err_t flash_unlock(flash_writer_t *fw);

esp_err_t flash_lock(flash_writer_t *fw);

esp_err_t flash_erase_page(flash_writer_t *fw, uint32_t page_addr);

esp_err_t flash_erase_all(flash_writer_t *fw);

esp_err_t flash_write_halfword(flash_writer_t *fw, uint32_t addr, uint16_t data);

esp_err_t flash_write_buffer(flash_writer_t *fw, uint32_t addr, const uint8_t *data, uint32_t len);

bool flash_verify(flash_writer_t *fw, uint32_t addr, const uint8_t *data, uint32_t len);

esp_err_t flash_read_id(flash_writer_t *fw, uint32_t *device_id, uint32_t *flash_size);

esp_err_t flash_mass_erase(flash_writer_t *fw);

#ifdef __cplusplus
}
#endif

#endif
