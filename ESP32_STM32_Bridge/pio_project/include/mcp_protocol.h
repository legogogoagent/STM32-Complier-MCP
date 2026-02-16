#pragma once

#include <stdint.h>

/**
 * @brief MCP v2 Binary Protocol Definition
 *        Little Endian
 */

#define MCP_MAGIC       0x324D4350  // 'MCP2'
#define MCP_VERSION     2

// --- Frame Header (16 Bytes) ---
typedef struct __attribute__((packed)) {
    uint32_t magic;          // MCP_MAGIC
    uint16_t version;        // MCP_VERSION
    uint16_t type;           // mcp_packet_type_t
    uint32_t seq;            // Sequence ID
    uint32_t length;         // Payload Length
    uint32_t crc32;          // CRC32(Header_no_CRC + Payload)
} mcp_header_t;

// --- Packet Types ---
typedef enum {
    // Commands (Client -> Server)
    MCP_CMD_AUTH            = 0x01,
    MCP_CMD_DISCONNECT      = 0x02,
    MCP_CMD_STATUS          = 0x03,
    MCP_CMD_RESET           = 0x04,
    MCP_CMD_IDCODE          = 0x05,
    MCP_CMD_HALT            = 0x06,
    
    // Flash Commands
    MCP_CMD_FLASH_BEGIN     = 0x10,
    MCP_CMD_FLASH_DATA      = 0x11,
    MCP_CMD_FLASH_END       = 0x12,
    
    // UART Commands
    MCP_CMD_UART_CONFIG     = 0x20,
    MCP_CMD_UART_START      = 0x21,
    MCP_CMD_UART_STOP       = 0x22,

    // Responses (Server -> Client)
    MCP_RESP_OK             = 0x80,
    MCP_RESP_ERROR          = 0x81,
    MCP_RESP_AUTH_CHALLENGE = 0x82,
    MCP_RESP_DATA           = 0x83,

    // Events (Server -> Client, Async)
    MCP_EVENT_LOG           = 0x90,
    MCP_EVENT_PROGRESS      = 0x91,
    MCP_EVENT_STATE         = 0x92,
} mcp_packet_type_t;

// --- Error Codes ---
typedef enum {
    MCP_ERR_NONE            = 0x00,
    MCP_ERR_UNKNOWN_CMD     = 0x01,
    MCP_ERR_AUTH_REQUIRED   = 0x02,
    MCP_ERR_AUTH_FAILED     = 0x03,
    MCP_ERR_SESSION_BUSY    = 0x04,
    MCP_ERR_TIMEOUT         = 0x05,
    MCP_ERR_CHECKSUM        = 0x06,
    MCP_ERR_FLASH_VERIFY    = 0x07,
    MCP_ERR_INTERNAL        = 0xFF,
} mcp_error_code_t;

// --- Payloads ---

// CMD_AUTH
typedef struct __attribute__((packed)) {
    uint8_t response[32];    // HMAC-SHA256(secret, nonce)
} mcp_payload_auth_t;

// RESP_AUTH_CHALLENGE
typedef struct __attribute__((packed)) {
    uint8_t nonce[16];       // 128-bit Random Nonce
} mcp_payload_challenge_t;

// CMD_FLASH_BEGIN
typedef struct __attribute__((packed)) {
    uint32_t address;
    uint32_t total_size;
    uint32_t chunk_size;
    uint8_t  erase_all;      // 1=Mass Erase, 0=Page Erase
} mcp_payload_flash_begin_t;

// CMD_FLASH_DATA
typedef struct __attribute__((packed)) {
    uint32_t offset;
    uint32_t data_len;
    uint8_t  data[];
} mcp_payload_flash_data_t;

// EVENT_PROGRESS
typedef struct __attribute__((packed)) {
    uint32_t current;
    uint32_t total;
    uint8_t  percent;
} mcp_payload_progress_t;
