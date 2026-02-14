#include <string.h>
#include <stdio.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "session_manager.h"
#include "http_server.h"

static const char *TAG = "HTTP";

static esp_err_t root_handler(httpd_req_t *req) {
    const char *html = 
        "<!DOCTYPE html>"
        "<html><head><meta charset='UTF-8'>"
        "<title>ESP32 STM32 Bridge</title>"
        "<style>"
        "body{font-family:Arial;margin:20px;background:#f5f5f5;}"
        ".header{background:linear-gradient(135deg,#667eea,#764ba2);color:white;padding:20px;border-radius:10px;}"
        ".card{background:white;padding:15px;margin:15px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"
        ".btn{background:#667eea;color:white;padding:8px 16px;border:none;border-radius:4px;margin:5px;}"
        "input,select{width:100%%;padding:8px;margin:5px 0;}"
        ".notice{background:#fff3e0;border:1px solid #ff9800;padding:10px;border-radius:4px;}"
        "</style></head><body>"
        
        "<div class='header'>"
        "<h1>ESP32-STM32 Bridge</h1>"
        "<p>无线 STM32 烧录调试器</p>"
        "</div>"
        
        "<div class='notice'>"
        "⚠️ <strong>只读模式</strong> - 按设备上的物理按键(GPIO10)开启授权后可操作"
        "</div>"
        
        "<div class='card'>"
        "<h2>设备状态</h2>"
        "<p>当前状态: <strong>空闲</strong></p>"
        "<p>WiFi: ESP32-Bridge-Setup</p>"
        "<p>IP: 192.168.4.1</p>"
        "</div>"
        
        "<div class='card'>"
        "<h2>串口配置</h2>"
        "<p>波特率: <select disabled><option>115200</option></select></p>"
        "<p>数据位: <select disabled><option>8</option></select></p>"
        "<p>停止位: <select disabled><option>1</option></select></p>"
        "<button class='btn' disabled>保存配置</button>"
        "</div>"
        
        "<div class='card'>"
        "<h2>调试工具</h2>"
        "<button class='btn' disabled>读取 IDCODE</button>"
        "<button class='btn' disabled>复位 STM32</button>"
        "</div>"
        
        "<div class='card'>"
        "<h2>系统维护</h2>"
        "<button class='btn' disabled>重启设备</button>"
        "</div>"
        
        "<div style='text-align:center;margin-top:20px;color:#666;font-size:12px;'>"
        "ESP32-STM32 Bridge v1.0</div>"
        "</body></html>";
    
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

void http_server_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 4096;
    
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        ESP_LOGI(TAG, "HTTP Server started on port 80");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
