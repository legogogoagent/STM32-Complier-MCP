#pragma once

#include "esp_err.h"

esp_err_t nvs_storage_init(void);
esp_err_t nvs_save_string(const char* key, const char* value);
esp_err_t nvs_load_string(const char* key, char* buffer, size_t max_len);
esp_err_t nvs_save_u32(const char* key, uint32_t value);
esp_err_t nvs_load_u32(const char* key, uint32_t* value);
