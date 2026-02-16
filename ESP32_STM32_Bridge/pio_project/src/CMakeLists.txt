idf_component_register(
    SRCS 
        "main.c"
        "gpio_driver.c"
        "nvs_storage.c"
        "session_manager.c"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash 
        driver 
        esp_timer 
        bridge_common
        esp_event
        esp_netif
)
