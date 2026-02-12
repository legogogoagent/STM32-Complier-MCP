#!/usr/bin/env python3
"""
Basic ESP32 STM32 Bridge Example

This example demonstrates basic usage of the ESP32 bridge for:
1. Connecting to the ESP32 bridge
2. Reading MCU IDCODE
3. Uploading firmware
4. Flashing the STM32
5. Verifying the flash

Hardware Setup:
- ESP32 GPIO18 -> STM32 SWDIO
- ESP32 GPIO19 -> STM32 SWCLK
- ESP32 GND -> STM32 GND
- Connect to ESP32 WiFi AP: ESP32-STM32-Bridge / stm32flash
"""

import sys
import os
import time

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))

try:
    from esp32_bridge_client import ESP32BridgeClient, BridgeError, FlashError
except ImportError:
    print("Error: Could not import esp32_bridge_client")
    print("Make sure the script is in ESP32_STM32_Bridge/scripts/")
    sys.exit(1)


def print_progress(percent: int, message: str):
    """Print flash progress"""
    bar_length = 40
    filled = int(bar_length * percent / 100)
    bar = '=' * filled + '-' * (bar_length - filled)
    print(f'\r[{bar}] {percent}% {message}', end='', flush=True)
    if percent >= 100:
        print()


def main():
    """Main example workflow"""
    
    # Configuration
    ESP32_HOST = "192.168.4.1"  # Default ESP32 AP IP
    ESP32_PORT = 4444
    FIRMWARE_FILE = "firmware.bin"  # Your firmware file
    
    print("=" * 60)
    print("ESP32 STM32 Bridge - Basic Flash Example")
    print("=" * 60)
    print()
    
    # Step 1: Connect to ESP32
    print(f"Step 1: Connecting to ESP32 at {ESP32_HOST}:{ESP32_PORT}...")
    client = ESP32BridgeClient(ESP32_HOST, ESP32_PORT)
    
    try:
        client.connect()
        print("✓ Connected successfully")
        print()
        
        # Step 2: Get version info
        print("Step 2: Getting bridge version...")
        version = client.get_version()
        print(f"✓ Bridge version: {version}")
        print()
        
        # Step 3: Reset and read IDCODE
        print("Step 3: Resetting SWD and reading IDCODE...")
        idcode = client.reset_and_idcode()
        print(f"✓ MCU IDCODE: 0x{idcode:08X}")
        
        # Decode IDCODE
        part_no = (idcode >> 12) & 0xFFF
        designer = (idcode >> 1) & 0x7FF
        print(f"  Part Number: 0x{part_no:03X}")
        print(f"  Designer: 0x{designer:03X}")
        
        # Identify MCU family
        families = {
            0x410: "STM32F1xx (Cortex-M3)",
            0x411: "STM32F2xx (Cortex-M3)",
            0x413: "STM32F4xx (Cortex-M4)",
            0x414: "STM32F1xx (Cortex-M3) [High density]",
            0x418: "STM32L1xx (Cortex-M3)",
            0x420: "STM32F0xx (Cortex-M0)",
            0x421: "STM32F4xx (Cortex-M4) [Dual core]",
            0x431: "STM32F4xx (Cortex-M4) [Advanced]",
            0x432: "STM32F3xx (Cortex-M4)",
            0x440: "STM32F0xx (Cortex-M0+)",
            0x444: "STM32L0xx (Cortex-M0+)",
            0x447: "STM32L0xx (Cortex-M0+) [Cat 5]",
            0x450: "STM32H7xx (Cortex-M7)",
            0x451: "STM32F7xx (Cortex-M7)",
            0x452: "STM32F7xx (Cortex-M7) [Dual core]",
            0x457: "STM32H7xx (Cortex-M7) [Dual core]",
            0x458: "STM32H7xx (Cortex-M7) [Single core]",
            0x464: "STM32L4xx (Cortex-M4)",
            0x470: "STM32L4xx (Cortex-M4) [Plus]",
            0x471: "STM32L4xx (Cortex-M4) [Cat 4]",
            0x480: "STM32H7xx (Cortex-M7) [Cat 3]",
            0x482: "STM32H7xx (Cortex-M7) [Cat 4]",
            0x490: "STM32WBxx (Cortex-M4/M0+)",
            0x495: "STM32WLxx (Cortex-M4)",
        }
        
        family = families.get(part_no, "Unknown MCU")
        print(f"  Detected: {family}")
        print()
        
        # Step 4: Check if firmware file exists
        if not os.path.exists(FIRMWARE_FILE):
            print(f"⚠ Firmware file not found: {FIRMWARE_FILE}")
            print("  Creating a test firmware file...")
            
            # Create a simple test firmware (blink LED pattern)
            test_firmware = bytes([
                0x00, 0x00, 0x00, 0x20,  # Initial SP
                0x09, 0x00, 0x00, 0x08,  # Reset vector
            ] + [0xFF] * 504)  # Padding to 512 bytes
            
            with open(FIRMWARE_FILE, 'wb') as f:
                f.write(test_firmware)
            print(f"✓ Created test firmware: {FIRMWARE_FILE} ({len(test_firmware)} bytes)")
            print()
        
        # Step 5: Read and upload firmware
        print(f"Step 5: Reading firmware from {FIRMWARE_FILE}...")
        with open(FIRMWARE_FILE, 'rb') as f:
            firmware = f.read()
        print(f"✓ Loaded {len(firmware)} bytes")
        print()
        
        print("Step 6: Uploading firmware to ESP32...")
        client.upload_firmware(firmware)
        print(f"✓ Uploaded {len(firmware)} bytes")
        print()
        
        # Step 7: Flash the STM32
        print("Step 7: Flashing STM32...")
        print("  This may take a while depending on firmware size...")
        start_time = time.time()
        
        result = client.flash_firmware(
            firmware,
            progress_callback=print_progress
        )
        
        elapsed = time.time() - start_time
        print(f"✓ Flash completed in {elapsed:.2f} seconds")
        print()
        
        # Step 8: Reset the MCU
        print("Step 8: Resetting MCU...")
        client.reset_target()
        print("✓ MCU reset complete")
        print()
        
        print("=" * 60)
        print("SUCCESS! Firmware flashed successfully!")
        print("=" * 60)
        print()
        print("Next steps:")
        print("  1. Check if your STM32 is running the new firmware")
        print("  2. Use serial monitor to see debug output:")
        print("     - STM32 TX -> ESP32 GPIO16")
        print("     - Connect to ESP32 and view output")
        print()
        
    except BridgeError as e:
        print()
        print("=" * 60)
        print("ERROR: Bridge communication failed")
        print("=" * 60)
        print(f"  {e}")
        print()
        print("Troubleshooting:")
        print("  1. Check WiFi connection to ESP32-STM32-Bridge")
        print("  2. Verify ESP32 is powered on")
        print("  3. Check if ESP32 is at the correct IP address")
        print("  4. Verify SWD connections (GPIO18/19)")
        sys.exit(1)
        
    except FlashError as e:
        print()
        print("=" * 60)
        print("ERROR: Flash programming failed")
        print("=" * 60)
        print(f"  {e}")
        print()
        print("Troubleshooting:")
        print("  1. Check SWD wiring (GPIO18->SWDIO, GPIO19->SWCLK)")
        print("  2. Ensure STM32 is powered")
        print("  3. Check if Flash is write-protected")
        print("  4. Try power cycling both devices")
        sys.exit(1)
        
    except Exception as e:
        print()
        print("=" * 60)
        print("ERROR: Unexpected error")
        print("=" * 60)
        print(f"  {type(e).__name__}: {e}")
        sys.exit(1)
        
    finally:
        client.disconnect()
        print("Disconnected from ESP32 bridge")


if __name__ == "__main__":
    main()
