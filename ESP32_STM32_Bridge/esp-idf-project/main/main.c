/**
 * @file main.c
 * @brief ESP32C3 STM32 Bridge 主程序入口
 * 
 * @details 这是一个基于 ESP32C3 的 STM32 无线烧录调试器，主要功能包括：
 *          - WiFi AP 模式，提供 Web 配置界面 (Landing Page)
 *          - TCP MCP 服务器，用于接收烧录命令
 *          - 物理按键授权机制，防止远程误操作
 *          - Session 管理，确保单会话独占
 * 
 * @author ESP32-STM32 Bridge Project
 * @date 2026-02-14
 * @version 1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "bridge_config.h"
#include "nvs_storage.h"
#include "gpio_driver.h"
#include "session_manager.h"
#include "wifi_manager.h"
#include "mcp_server.h"
#include "http_server.h"
#include "soft_uart.h"
#include "websocket_server.h"

static const char *TAG = "MAIN";

/**
 * @brief 应用程序主入口
 * 
 * @details 初始化流程：
 *          1. NVS 存储 - 保存配置和设备密钥
 *          2. GPIO 驱动 - LED 指示和授权按键
 *          3. Session 管理器 - 状态机和安全控制
 *          4. WiFi AP - 创建热点供用户连接
 *          5. MCP 服务器 - TCP 命令接口 (端口 4444)
 *          6. HTTP 服务器 - Web 配置界面 (端口 80)
 */
void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32-STM32 Bridge v1.0");
    ESP_LOGI(TAG, "Build date: %s %s", __DATE__, __TIME__);

    /* 1. 初始化 NVS (Non-Volatile Storage) - 用于保存 WiFi 配置和设备密钥 */
    ESP_ERROR_CHECK(nvs_storage_init());
    
    /* 2. 初始化 GPIO - 配置 LED 和授权按键 */
    gpio_driver_init();
    
    /* 3. 初始化 Session 管理器 - 管理 DISARMED/ARMED/OWNED/BURNING 状态 */
    ESP_ERROR_CHECK(session_init());
    
    /* 4. 初始化 WiFi 并启动 AP 模式
     *    SSID: ESP32-Bridge-Setup
     *    Password: stm32bridge
     *    IP: 192.168.4.1
     */
    wifi_manager_init();
    wifi_start_ap();
    
    /* 5. 启动 MCP TCP 服务器 - 接收烧录和调试命令 (端口 4444) */
    mcp_server_init();
    
    /* 6. 启动 HTTP 服务器 - 提供 Web 配置界面 (端口 80) */
    http_server_init();

    /* 7. 初始化 Soft UART - 软串口驱动 (GPIO0/1) */
    ESP_LOGI(TAG, "Initializing Soft UART on GPIO%d/%d", GPIO_UART_TX, GPIO_UART_RX);

    /* 8. 启动 WebSocket 服务器 - 浏览器串口透传 (端口 8080) */
    websocket_server_init();

    ESP_LOGI(TAG, "===============================================");
    ESP_LOGI(TAG, "System initialized successfully!");
    ESP_LOGI(TAG, "Connect to WiFi: ESP32-Bridge-Setup");
    ESP_LOGI(TAG, "Password: stm32bridge");
    ESP_LOGI(TAG, "Then open: http://192.168.4.1");
    ESP_LOGI(TAG, "Press GPIO10 button to enable control");
    ESP_LOGI(TAG, "===============================================");
    
    /* 主循环 - 系统运行中，所有工作由 FreeRTOS 任务处理 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
