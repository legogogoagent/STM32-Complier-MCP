#!/usr/bin/env python3
"""
MCP Flash Server Integration Example

This example demonstrates how to use the ESP32 bridge through the MCP Flash Server.
This is the recommended way to flash STM32 devices as part of the MCP workflow.

Prerequisites:
1. ESP32 flashed with bridge firmware
2. STM32 connected to ESP32 (GPIO18/19)
3. MCP Flash Server running
4. PC connected to ESP32 WiFi AP

Usage:
    python mcp_integration_example.py --workspace /path/to/project --hex firmware.hex

Or use the MCP tool directly:
    mcp_flash.flash_firmware_esp32(
        workspace="/path/to/project",
        hex_file="firmware.hex",
        esp32_host="192.168.4.1"
    )
"""

import argparse
import sys
import os
import json
from pathlib import Path

# MCP Flash Server integration
try:
    # Try to import from installed package
    from mcp_flash.stm32_flash_server import create_flash_mcp
    from mcp_flash.esp32_remote_flasher import ESP32RemoteFlasher
    MCP_AVAILABLE = True
except ImportError:
    MCP_AVAILABLE = False
    print("Warning: MCP Flash Server not available")
    print("Install with: pip install -e .")


def discover_esp32_devices(subnet: str = "192.168.4", timeout: float = 2.0):
    """
    Discover ESP32 bridges on the network.
    
    Args:
        subnet: IP subnet to scan (e.g., "192.168.4")
        timeout: Timeout per host in seconds
        
    Returns:
        List of discovered devices with info
    """
    import socket
    
    devices = []
    print(f"Scanning {subnet}.x network for ESP32 bridges...")
    
    # Scan common IP range (skip .0, .1, .255)
    for host in range(2, 255):
        ip = f"{subnet}.{host}"
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(timeout)
            result = sock.connect_ex((ip, 4444))
            if result == 0:
                # Try to get version info
                try:
                    sock.send(b"version\n")
                    response = sock.recv(256).decode('utf-8').strip()
                    devices.append({
                        'ip': ip,
                        'port': 4444,
                        'version': response
                    })
                    print(f"  Found: {ip} - {response}")
                except:
                    devices.append({
                        'ip': ip,
                        'port': 4444,
                        'version': 'Unknown'
                    })
                    print(f"  Found: {ip} (version unknown)")
            sock.close()
        except:
            pass
    
    return devices


def flash_via_mcp(workspace: str, hex_file: str, esp32_host: str = "192.168.4.1"):
    """
    Flash firmware using MCP Flash Server.
    
    This demonstrates how to call the MCP tool programmatically.
    In production, this would be called by the AI Agent.
    """
    if not MCP_AVAILABLE:
        print("Error: MCP Flash Server not available")
        return False
    
    print("=" * 60)
    print("MCP Flash Server - ESP32 Integration")
    print("=" * 60)
    print()
    
    # Create flasher instance
    flasher = ESP32RemoteFlasher(host=esp32_host, port=4444)
    
    try:
        # Check connection
        print(f"Step 1: Checking ESP32 bridge at {esp32_host}...")
        if not flasher.connect():
            print("✗ Failed to connect to ESP32 bridge")
            return False
        print("✓ Connected")
        print()
        
        # Read IDCODE
        print("Step 2: Reading MCU IDCODE...")
        idcode = flasher.read_idcode()
        family = flasher.detect_mcu_family(idcode)
        print(f"✓ IDCODE: 0x{idcode:08X} ({family})")
        print()
        
        # Read hex file
        print(f"Step 3: Reading hex file: {hex_file}")
        hex_path = Path(workspace) / "out" / hex_file
        if not hex_path.exists():
            print(f"✗ Hex file not found: {hex_path}")
            return False
        
        with open(hex_path, 'r') as f:
            hex_content = f.read()
        print(f"✓ Loaded {len(hex_content)} bytes of hex data")
        print()
        
        # Convert to binary
        print("Step 4: Converting Intel HEX to binary...")
        from mcp_flash.stm32_flash_server import _convert_hex_to_bin
        binary = _convert_hex_to_bin(hex_content)
        print(f"✓ Converted to {len(binary)} bytes of binary")
        print()
        
        # Flash
        print("Step 5: Flashing firmware...")
        def progress_callback(phase: str, current: int, total: int):
            percent = int(100 * current / total) if total > 0 else 0
            print(f"\r  {phase}: {percent}% ({current}/{total} bytes)", end='', flush=True)
        
        success = flasher.flash_firmware(binary, progress_callback=progress_callback)
        print()  # New line after progress
        
        if success:
            print("✓ Flash completed successfully")
        else:
            print("✗ Flash failed")
            return False
        
        print()
        print("=" * 60)
        print("SUCCESS!")
        print("=" * 60)
        return True
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        return False
    finally:
        flasher.disconnect()


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='ESP32 STM32 Bridge - MCP Integration Example'
    )
    parser.add_argument(
        '--discover',
        action='store_true',
        help='Discover ESP32 devices on network'
    )
    parser.add_argument(
        '--workspace',
        type=str,
        default='.',
        help='Project workspace directory'
    )
    parser.add_argument(
        '--hex',
        type=str,
        default='firmware.hex',
        help='Hex filename (relative to out/)'
    )
    parser.add_argument(
        '--host',
        type=str,
        default='192.168.4.1',
        help='ESP32 IP address'
    )
    parser.add_argument(
        '--subnet',
        type=str,
        default='192.168.4',
        help='Subnet for device discovery'
    )
    
    args = parser.parse_args()
    
    if args.discover:
        # Discovery mode
        devices = discover_esp32_devices(args.subnet)
        print()
        print(f"Found {len(devices)} device(s)")
        for dev in devices:
            print(f"  - {dev['ip']}:{dev['port']} ({dev['version']})")
        return
    
    # Flash mode
    success = flash_via_mcp(args.workspace, args.hex, args.host)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
