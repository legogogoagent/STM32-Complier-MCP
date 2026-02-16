#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs_storage.h"
#include "gpio_driver.h"
#include "session_manager.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32-STM32 Bridge...");

    // 1. Initialize NVS
    ESP_ERROR_CHECK(nvs_storage_init());

    // 2. Initialize GPIO (LEDs + Button)
    gpio_driver_init();

    // 3. Initialize Session Manager
    ESP_ERROR_CHECK(session_init());

    // 4. (Future) Initialize WiFi & mDNS
    // wifi_manager_init();

    // 5. (Future) Initialize MCP Server
    // mcp_server_init();

    ESP_LOGI(TAG, "System initialized. Waiting for button press...");
    
    // Main loop (Health check / Watchdog)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        // ESP_LOGI(TAG, "Heartbeat - Free Heap: %lu", esp_get_free_heap_size());
    }
}
