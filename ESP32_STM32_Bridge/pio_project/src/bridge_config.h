#pragma once

#include <stdint.h>

/**
 * @brief ESP32C3 Super Mini Hardware Configuration
 */

// ============================================================================
// GPIO Assignments
// ============================================================================

// --- System ---
#define GPIO_SYSTEM_LED         2       // D0: Green LED (Status)
#define GPIO_WIFI_LED           8       // Onboard Blue LED (WiFi Status)
#define GPIO_AUTH_BUTTON        10      // D6: Physical Auth Button (Active Low, Pull-up)

// --- SWD Interface ---
#define GPIO_SWDIO              3       // D1: SWD Data
#define GPIO_SWCLK              4       // D2: SWD Clock
#define GPIO_NRST               5       // D3: Target Reset (Active Low)

// --- UART Bridge ---
#define GPIO_UART_RX            6       // D4: RX (Connect to Target TX)
#define GPIO_UART_TX            7       // D5: TX (Connect to Target RX)

// --- USB Debug (Native USB) ---
#define GPIO_USB_DP             19      // Internal USB D+
#define GPIO_USB_DN             18      // Internal USB D-
// Note: GPIO20/21 are U0RXD/U0TXD for external USB-TTL if used

// ============================================================================
// Session Parameters
// ============================================================================

#define ARMED_WINDOW_MS         60000   // 60 seconds auth window
#define AUTH_FAIL_MAX           5       // Max auth failures before lockout
#define LOCKOUT_DURATION_S      300     // 5 minutes lockout
#define SESSION_TIMEOUT_S       120     // 2 minutes inactivity timeout

// ============================================================================
// Buffer Sizes
// ============================================================================

#define FIRMWARE_CHUNK_SIZE     4096    // 4KB stream chunk
#define UART_RING_BUF_SIZE      2048    // UART buffer
#define WS_TX_BUF_SIZE          1024    // WebSocket TX buffer

// ============================================================================
// NVS Keys
// ============================================================================

#define NVS_NAMESPACE           "stm32bridge"
#define NVS_KEY_CONFIG          "config_v1"
#define NVS_KEY_SECRET          "dev_secret"
