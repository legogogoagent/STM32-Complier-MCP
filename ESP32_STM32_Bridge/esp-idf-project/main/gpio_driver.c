#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bridge_config.h"
#include "gpio_driver.h"
#include "session_manager.h"

static volatile led_state_t current_led_state = LED_STATE_OFF;
static TaskHandle_t led_task_handle = NULL;

// --- Button ISR ---

static void IRAM_ATTR button_isr_handler(void* arg) {
    // Simple debounce logic could be added here or in the task
    // For now, we notify the session manager directly via a hook
    // Ideally, send a message to a queue, but session manager is simple
    // We'll trust the session manager to handle debounce or state checks
    // BUT: Calling session logic from ISR is risky.
    // BETTER: Notify a task. We'll reuse the LED task for button polling or simple check
}

// Actually, ESP-IDF recommends handling logic in a task.
// Let's create a dedicated input task or piggyback.
// Simplified: Polling for button in a low-priority task is robust for 60s windows.
// Let's stick to ISR -> Task notification.

static void button_task(void *arg) {
    uint32_t last_press = 0;
    while (1) {
        if (gpio_get_level(GPIO_AUTH_BUTTON) == 0) {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - last_press > 500) { // 500ms debounce
                session_handle_button_press();
                last_press = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz poll is enough for human button
    }
}

// --- LED Task ---

static void led_task(void *arg) {
    while (1) {
        switch (current_led_state) {
            case LED_STATE_OFF:
                gpio_set_level(GPIO_SYSTEM_LED, 0);
                gpio_set_level(GPIO_WIFI_LED, 1); // Blue LED usually active low? Assume active high for now
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            case LED_STATE_ON:
                gpio_set_level(GPIO_SYSTEM_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            case LED_STATE_FAST_BLINK: // ARMED
                gpio_set_level(GPIO_WIFI_LED, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(GPIO_WIFI_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            case LED_STATE_BREATHE: // BURNING
                gpio_set_level(GPIO_WIFI_LED, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(GPIO_WIFI_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            default:
                vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

// --- Init ---

void gpio_driver_init(void) {
    // Configure LEDs
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_SYSTEM_LED) | (1ULL << GPIO_WIFI_LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configure Button
    io_conf.intr_type = GPIO_INTR_DISABLE; // Polling mode
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_AUTH_BUTTON);
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    xTaskCreate(led_task, "led_task", 2048, NULL, 1, &led_task_handle);
    xTaskCreate(button_task, "btn_task", 2048, NULL, 1, NULL);
}

void gpio_set_led_state(led_state_t state) {
    current_led_state = state;
}
