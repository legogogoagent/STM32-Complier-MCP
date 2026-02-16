#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "session_manager.h"
#include "soft_uart.h"
#include "bridge_config.h"
#include "http_server.h"

static const char *TAG = "HTTP";

extern soft_uart_handle_t s_uart_handle;
extern bool s_uart_initialized;

static esp_err_t root_handler(httpd_req_t *req) {
    const char *html = 
        "<!DOCTYPE html>"
        "<html><head><meta charset='UTF-8'>"
        "<title>ESP32 STM32 Bridge</title>"
        "<style>"
        "body{font-family:monospace;margin:20px;background:#1e1e1e;color:#ddd;}"
        ".header{background:linear-gradient(135deg,#667eea,#764ba2);color:white;padding:20px;border-radius:10px;}"
        ".card{background:#2d2d2d;padding:15px;margin:15px 0;border-radius:8px;}"
        "input,select,textarea{width:100%%;padding:8px;margin:5px 0;background:#3d3d3d;color:#fff;border:1px solid #555;}"
        "button{background:#667eea;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px;}"
        "button:hover{background:#5a6fd6;}"
        "#terminal{background:#000;color:#0f0;padding:10px;height:200px;overflow-y:auto;border-radius:4px;font-size:12px;}"
        ".row{display:flex;gap:10px;}"
        ".col{flex:1;}"
        "</style></head><body>"
        
        "<div class='header'>"
        "<h1>ESP32-STM32 Bridge</h1>"
        "<p>无线 STM32 烧录调试器</p>"
        "</div>"
        
        "<div class='card'>"
        "<h2>串口终端</h2>"
        "<div class='row'>"
        "<div class='col'>"
        "<label>波特率:</label>"
        "<select id='baud'>"
        "<option value='9600'>9600</option>"
        "<option value='19200'>19200</option>"
        "<option value='38400'>38400</option>"
        "<option value='57600'>57600</option>"
        "<option value='115200' selected>115200</option>"
        "</select>"
        "</div>"
        "<div class='col'>"
        "<button onclick='startUart()'>启动串口</button>"
        "<button onclick='stopUart()'>停止串口</button>"
        "</div>"
        "</div>"
        "<div id='terminal'></div>"
        "<div class='row'>"
        "<input type='text' id='input' placeholder='输入文本或hex(如 Hello 或 48656C6C6F)' onkeypress='if(event.key==\"Enter\")sendData()'>"
        "<button onclick='sendData()'>发送</button>"
        "<button onclick='clearTerm()'>清屏</button>"
        "</div>"
        "</div>"
        
        "<div class='card'>"
        "<h2>设备信息</h2>"
        "<p>WiFi: ESP32-Bridge-Setup</p>"
        "<p>IP: 192.168.4.1</p>"
        "<p>MCP端口: 4444</p>"
        "</div>"
        
        "<script>"
        "var uartRunning=false;"
        "var pollInt=null;"
        "function log(msg){document.getElementById('terminal').innerHTML+=msg+'<br>';document.getElementById('terminal').scrollTop=document.getElementById('terminal').scrollHeight;}"
        "function startUart(){"
        "  var baud=document.getElementById('baud').value;"
        "  fetch('/uart_start?baud='+baud).then(r=>r.text()).then(t=>{"
        "    log('[UART] '+t);"
        "    uartRunning=true;"
        "    pollInt=setInterval(pollRecv,500);"
        "  });"
        "}"
        "function stopUart(){"
        "  fetch('/uart_stop').then(r=>r.text()).then(t=>{"
        "    log('[UART] '+t);"
        "    uartRunning=false;"
        "    if(pollInt){clearInterval(pollInt);pollInt=null;}"
        "  });"
        "}"
        "function sendData(){"
        "  var data=document.getElementById('input').value;"
        "  if(!data)return;"
        "  fetch('/uart_send?data='+encodeURIComponent(data)).then(r=>r.text()).then(t=>{"
        "    log('[TX] '+data);"
        "    document.getElementById('input').value='';"
        "  });"
        "}"
        "function pollRecv(){"
        "  fetch('/uart_recv').then(r=>r.json()).then(j=>{"
        "    if(j.status==='ok' && j.data){log('[RX] '+j.hex+' ('+j.data+')');}"
        "  }).catch(e=>{});"
        "}"
        "function clearTerm(){document.getElementById('terminal').innerHTML='';}"
        "log('系统就绪');"
        "</script>"
        
        "<div style='text-align:center;margin-top:20px;color:#666;font-size:12px;'>"
        "ESP32-STM32 Bridge v1.1</div>"
        "</body></html>";
    
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

static esp_err_t uart_start_handler(httpd_req_t *req)
{
    char buf[32];
    int baud = 115200;
    
    if (httpd_query_key_value(req->uri, "baud", buf, sizeof(buf)) == ESP_OK) {
        baud = atoi(buf);
    }
    
    if (!s_uart_initialized) {
        soft_uart_init(&s_uart_handle, GPIO_UART_TX, GPIO_UART_RX, baud);
        s_uart_initialized = true;
    }
    
    soft_uart_set_baudrate(&s_uart_handle, baud);
    soft_uart_start(&s_uart_handle);
    
    char resp[64];
    sprintf(resp, "{\"status\":\"ok\",\"baud\":%d}", baud);
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

static esp_err_t uart_stop_handler(httpd_req_t *req)
{
    if (s_uart_initialized) {
        soft_uart_stop(&s_uart_handle);
    }
    httpd_resp_send(req, "{\"status\":\"ok\"}", -1);
    return ESP_OK;
}

static esp_err_t uart_send_handler(httpd_req_t *req)
{
    char buf[512];
    char hex_buf[256];
    
    if (httpd_query_key_value(req->uri, "data", buf, sizeof(buf)) != ESP_OK) {
        httpd_resp_send(req, "{\"status\":\"error\",\"msg\":\"No data\"}", -1);
        return ESP_FAIL;
    }
    
    size_t len = strlen(buf);
    size_t hex_len = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '%' && i + 2 < len) {
            char hex[3] = {buf[i+1], buf[i+2], 0};
            hex_buf[hex_len++] = strtol(hex, NULL, 16);
            i += 2;
        } else {
            hex_buf[hex_len++] = buf[i];
        }
    }
    
    if (s_uart_initialized) {
        soft_uart_write(&s_uart_handle, (uint8_t *)hex_buf, hex_len);
    }
    
    httpd_resp_send(req, "{\"status\":\"ok\",\"sent\":1}", -1);
    return ESP_OK;
}

static esp_err_t uart_recv_handler(httpd_req_t *req)
{
    char buf[512];
    char *ptr = buf;
    
    if (!s_uart_initialized) {
        httpd_resp_send(req, "{\"status\":\"error\",\"msg\":\"UART not initialized\"}", -1);
        return ESP_OK;
    }
    
    uint8_t data[128];
    int len = soft_uart_read(&s_uart_handle, data, sizeof(data));
    
    if (len <= 0) {
        httpd_resp_send(req, "{\"status\":\"ok\",\"data\":\"\",\"hex\":\"\"}", -1);
        return ESP_OK;
    }
    
    char hex_buf[256] = {0};
    char ascii_buf[128] = {0};
    
    for (int i = 0; i < len; i++) {
        sprintf(hex_buf + strlen(hex_buf), "%02X", data[i]);
        if (data[i] >= 32 && data[i] <= 126) {
            sprintf(ascii_buf + strlen(ascii_buf), "%c", data[i]);
        } else {
            sprintf(ascii_buf + strlen(ascii_buf), ".");
        }
    }
    
    sprintf(buf, "{\"status\":\"ok\",\"data\":\"%s\",\"hex\":\"%s\",\"len\":%d}", ascii_buf, hex_buf, len);
    
    httpd_resp_send(req, buf, strlen(buf));
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
        
        httpd_uri_t uart_start_uri = {
            .uri = "/uart_start",
            .method = HTTP_GET,
            .handler = uart_start_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uart_start_uri);
        
        httpd_uri_t uart_stop_uri = {
            .uri = "/uart_stop",
            .method = HTTP_GET,
            .handler = uart_stop_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uart_stop_uri);
        
        httpd_uri_t uart_send_uri = {
            .uri = "/uart_send",
            .method = HTTP_GET,
            .handler = uart_send_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uart_send_uri);
        
        httpd_uri_t uart_recv_uri = {
            .uri = "/uart_recv",
            .method = HTTP_GET,
            .handler = uart_recv_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uart_recv_uri);
        
        ESP_LOGI(TAG, "HTTP Server started on port 80");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
