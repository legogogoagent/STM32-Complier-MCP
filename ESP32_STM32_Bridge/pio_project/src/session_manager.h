#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Session States
typedef enum {
    SESSION_STATE_DISARMED = 0, // Idle, rejects control commands
    SESSION_STATE_ARMED,        // Auth window open (60s)
    SESSION_STATE_OWNED,        // Connected and authenticated
    SESSION_STATE_BURNING       // Flashing in progress
} session_state_t;

// Public API
esp_err_t session_init(void);
session_state_t session_get_state(void);
const char* session_get_owner_ip(void);

// Events
void session_handle_button_press(void);
bool session_handle_auth_request(const char* ip);
void session_handle_disconnect(void);
void session_set_burning(bool burning);

// Security Check
bool session_is_action_allowed(void);
