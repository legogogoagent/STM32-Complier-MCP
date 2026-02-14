#pragma once

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
    LED_STATE_FAST_BLINK,   // ARMED (10Hz)
    LED_STATE_SLOW_BLINK,   // (Reserved)
    LED_STATE_BREATHE       // BURNING (Fade in/out or 1Hz blink)
} led_state_t;

void gpio_driver_init(void);
void gpio_set_led_state(led_state_t state);
