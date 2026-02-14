#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "bridge_config.h"
#include "session_manager.h"
#include "gpio_driver.h" // Will implement next

static const char *TAG = "SESSION";

static struct {
    session_state_t state;
    char owner_ip[16];
    TimerHandle_t armed_timer;
    TimerHandle_t idle_timer;
    int64_t last_activity;
} session_ctx;

// --- Timer Callbacks ---

static void armed_timeout_cb(TimerHandle_t xTimer) {
    if (session_ctx.state == SESSION_STATE_ARMED) {
        ESP_LOGI(TAG, "Auth window expired");
        session_ctx.state = SESSION_STATE_DISARMED;
        gpio_set_led_state(LED_STATE_OFF);
    }
}

static void idle_timeout_cb(TimerHandle_t xTimer) {
    if (session_ctx.state == SESSION_STATE_OWNED) {
        int64_t now = esp_timer_get_time();
        if ((now - session_ctx.last_activity) > (SESSION_TIMEOUT_S * 1000000LL)) {
            ESP_LOGW(TAG, "Session timed out");
            session_handle_disconnect();
        }
    }
}

// --- Public API ---

esp_err_t session_init(void) {
    memset(&session_ctx, 0, sizeof(session_ctx));
    session_ctx.state = SESSION_STATE_DISARMED;
    
    session_ctx.armed_timer = xTimerCreate("armed_tmr", 
        pdMS_TO_TICKS(ARMED_WINDOW_MS), pdFALSE, NULL, armed_timeout_cb);
        
    session_ctx.idle_timer = xTimerCreate("idle_tmr", 
        pdMS_TO_TICKS(10000), pdTRUE, NULL, idle_timeout_cb); // Check every 10s
        
    xTimerStart(session_ctx.idle_timer, 0);
    return ESP_OK;
}

session_state_t session_get_state(void) {
    return session_ctx.state;
}

const char* session_get_owner_ip(void) {
    return session_ctx.owner_ip;
}

void session_handle_button_press(void) {
    // Only allow DISARMED -> ARMED transition via button
    if (session_ctx.state == SESSION_STATE_DISARMED) {
        ESP_LOGI(TAG, "Button pressed: ARMED window open");
        session_ctx.state = SESSION_STATE_ARMED;
        xTimerStart(session_ctx.armed_timer, 0);
        gpio_set_led_state(LED_STATE_FAST_BLINK);
    } 
    // Optional: Long press to force disconnect (future feature)
}

bool session_handle_auth_request(const char* ip) {
    if (session_ctx.state == SESSION_STATE_ARMED) {
        ESP_LOGI(TAG, "Auth successful from %s", ip);
        session_ctx.state = SESSION_STATE_OWNED;
        strncpy(session_ctx.owner_ip, ip, sizeof(session_ctx.owner_ip) - 1);
        xTimerStop(session_ctx.armed_timer, 0);
        session_ctx.last_activity = esp_timer_get_time();
        gpio_set_led_state(LED_STATE_ON);
        return true;
    }
    ESP_LOGW(TAG, "Auth rejected (State: %d)", session_ctx.state);
    return false;
}

void session_handle_disconnect(void) {
    if (session_ctx.state != SESSION_STATE_DISARMED) {
        ESP_LOGI(TAG, "Session disconnected");
        session_ctx.state = SESSION_STATE_DISARMED;
        memset(session_ctx.owner_ip, 0, sizeof(session_ctx.owner_ip));
        gpio_set_led_state(LED_STATE_OFF);
    }
}

void session_set_burning(bool burning) {
    if (session_ctx.state == SESSION_STATE_OWNED && burning) {
        session_ctx.state = SESSION_STATE_BURNING;
        gpio_set_led_state(LED_STATE_BREATHE);
    } else if (session_ctx.state == SESSION_STATE_BURNING && !burning) {
        session_ctx.state = SESSION_STATE_OWNED;
        gpio_set_led_state(LED_STATE_ON);
    }
    session_ctx.last_activity = esp_timer_get_time();
}

bool session_is_action_allowed(void) {
    session_ctx.last_activity = esp_timer_get_time(); // Touch watchdog
    return (session_ctx.state == SESSION_STATE_OWNED || 
            session_ctx.state == SESSION_STATE_BURNING);
}
