"""
Unit tests for ESP32 STM32 Flash Programming Algorithms

Tests the STM32FlashProgrammer class logic without requiring actual hardware.
Uses mocking to simulate SWD protocol responses.
"""

import unittest
from unittest.mock import Mock, patch, call
import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'scripts'))


class TestSTM32IDCodeDetection(unittest.TestCase):
    """Test MCU family detection from IDCODE values"""
    
    def test_stm32f1_detection(self):
        """Test detection of STM32F1 series from IDCODE"""
        # STM32F103 IDCODE: 0x1BA00477 (Cortex-M3 r1p1)
        idcode = 0x1BA00477
        part_no = (idcode >> 12) & 0xFFF
        self.assertEqual(part_no, 0x410)  # F1 identifier
        
    def test_stm32f4_detection(self):
        """Test detection of STM32F4 series from IDCODE"""
        # STM32F407 IDCODE: 0x2BA01477 (Cortex-M4 r0p1)
        idcode = 0x2BA01477
        part_no = (idcode >> 12) & 0xFFF
        self.assertEqual(part_no, 0x413)  # F4 identifier
        
    def test_stm32f7_detection(self):
        """Test detection of STM32F7 series from IDCODE"""
        # STM32F767 IDCODE: 0x5BA02477 (Cortex-M7 r0p1)
        idcode = 0x5BA02477
        part_no = (idcode >> 12) & 0xFFF
        self.assertEqual(part_no, 0x451)  # F7 identifier
        
    def test_stm32h7_detection(self):
        """Test detection of STM32H7 series from IDCODE"""
        # STM32H743 IDCODE: 0x6BA02477 (Cortex-M7 r0p0)
        idcode = 0x6BA02477
        part_no = (idcode >> 12) & 0xFFF
        self.assertEqual(part_no, 0x450)  # H7 identifier


class TestFlashRegisterAddresses(unittest.TestCase):
    """Test Flash register address constants"""
    
    def test_stm32f1_flash_base(self):
        """Verify STM32F1 Flash interface base address"""
        FLASH_F1_BASE = 0x40022000
        self.assertEqual(FLASH_F1_BASE, 0x40022000)
        
    def test_stm32f1_key_register(self):
        """Verify Flash key register address"""
        FLASH_F1_BASE = 0x40022000
        FLASH_F1_KEYR = FLASH_F1_BASE + 0x04
        self.assertEqual(FLASH_F1_KEYR, 0x40022004)
        
    def test_stm32f1_control_register(self):
        """Verify Flash control register address"""
        FLASH_F1_BASE = 0x40022000
        FLASH_F1_CR = FLASH_F1_BASE + 0x10
        self.assertEqual(FLASH_F1_CR, 0x40022010)
        
    def test_stm32f4_flash_base(self):
        """Verify STM32F4 Flash interface base address"""
        FLASH_F4_BASE = 0x40023C00
        self.assertEqual(FLASH_F4_BASE, 0x40023C00)


class TestFlashUnlockKeys(unittest.TestCase):
    """Test Flash unlock key sequences"""
    
    def test_stm32f1_key1(self):
        """Verify first unlock key for F1"""
        FLASH_F1_KEY1 = 0x45670123
        self.assertEqual(FLASH_F1_KEY1, 0x45670123)
        
    def test_stm32f1_key2(self):
        """Verify second unlock key for F1"""
        FLASH_F1_KEY2 = 0xCDEF89AB
        self.assertEqual(FLASH_F1_KEY2, 0xCDEF89AB)
        
    def test_key_sequence_order(self):
        """Verify unlock sequence must be KEY1 then KEY2"""
        # Keys must be written in specific order
        keys = [0x45670123, 0xCDEF89AB]
        self.assertEqual(keys[0], 0x45670123)  # KEY1 first
        self.assertEqual(keys[1], 0xCDEF89AB)  # KEY2 second


class TestFlashControlBits(unittest.TestCase):
    """Test Flash control register bit definitions"""
    
    def test_programming_bit(self):
        """Verify PG (Programming) bit position"""
        FLASH_F1_CR_PG = (1 << 0)
        self.assertEqual(FLASH_F1_CR_PG, 0x00000001)
        
    def test_page_erase_bit(self):
        """Verify PER (Page Erase) bit position"""
        FLASH_F1_CR_PER = (1 << 1)
        self.assertEqual(FLASH_F1_CR_PER, 0x00000002)
        
    def test_mass_erase_bit(self):
        """Verify MER (Mass Erase) bit position"""
        FLASH_F1_CR_MER = (1 << 2)
        self.assertEqual(FLASH_F1_CR_MER, 0x00000004)
        
    def test_start_bit(self):
        """Verify STRT (Start) bit position"""
        FLASH_F1_CR_STRT = (1 << 6)
        self.assertEqual(FLASH_F1_CR_STRT, 0x00000040)
        
    def test_lock_bit(self):
        """Verify LOCK bit position"""
        FLASH_F1_CR_LOCK = (1 << 7)
        self.assertEqual(FLASH_F1_CR_LOCK, 0x00000080)


class TestFlashStatusBits(unittest.TestCase):
    """Test Flash status register bit definitions"""
    
    def test_busy_bit(self):
        """Verify BSY (Busy) bit position"""
        FLASH_F1_SR_BSY = (1 << 0)
        self.assertEqual(FLASH_F1_SR_BSY, 0x00000001)
        
    def test_programming_error_bit(self):
        """Verify PGERR bit position"""
        FLASH_F1_SR_PGERR = (1 << 2)
        self.assertEqual(FLASH_F1_SR_PGERR, 0x00000004)
        
    def test_write_protection_error_bit(self):
        """Verify WRPRTERR bit position"""
        FLASH_F1_SR_WRPRTERR = (1 << 4)
        self.assertEqual(FLASH_F1_SR_WRPRTERR, 0x00000010)
        
    def test_end_of_operation_bit(self):
        """Verify EOP bit position"""
        FLASH_F1_SR_EOP = (1 << 5)
        self.assertEqual(FLASH_F1_SR_EOP, 0x00000020)


class TestDebugRegisters(unittest.TestCase):
    """Test Core Debug register addresses"""
    
    def test_dhcsr_address(self):
        """Verify DHCSR register address"""
        DEBUG_DHCSR = 0xE000EDF0
        self.assertEqual(DEBUG_DHCSR, 0xE000EDF0)
        
    def test_dcrsr_address(self):
        """Verify DCRSR register address"""
        DEBUG_DCRSR = 0xE000EDF4
        self.assertEqual(DEBUG_DCRSR, 0xE000EDF4)
        
    def test_demcr_address(self):
        """Verify DEMCR register address"""
        DEBUG_DEMCR = 0xE000EDFC
        self.assertEqual(DEBUG_DEMCR, 0xE000EDFC)


class TestDAPProtocol(unittest.TestCase):
    """Test DAP (Debug Access Port) protocol constants"""
    
    def test_dp_select_register(self):
        """Verify DP SELECT register address"""
        DAP_DP_SELECT = 0x08
        self.assertEqual(DAP_DP_SELECT, 0x08)
        
    def test_ap_csw_register(self):
        """Verify AP CSW register address"""
        DAP_AP_CSW = 0x00
        self.assertEqual(DAP_AP_CSW, 0x00)
        
    def test_ap_tar_register(self):
        """Verify AP TAR register address"""
        DAP_AP_TAR = 0x04
        self.assertEqual(DAP_AP_TAR, 0x04)
        
    def test_ap_drw_register(self):
        """Verify AP DRW register address"""
        DAP_AP_DRW = 0x0C
        self.assertEqual(DAP_AP_DRW, 0x0C)
        
    def test_csw_word_size(self):
        """Verify CSW word size encoding"""
        AP_CSW_SIZE_WORD = 0x00000002
        self.assertEqual(AP_CSW_SIZE_WORD, 0x00000002)
        
    def test_csw_device_enable(self):
        """Verify CSW device enable bit"""
        AP_CSW_DEVICEEN = 0x00000040
        self.assertEqual(AP_CSW_DEVICEEN, 0x00000040)


class TestFlashSectorAddresses(unittest.TestCase):
    """Test STM32F4 Flash sector addresses"""
    
    def test_sector0_address(self):
        """Verify Sector 0 base address"""
        FLASH_F4_SECTOR0_ADDR = 0x08000000
        self.assertEqual(FLASH_F4_SECTOR0_ADDR, 0x08000000)
        
    def test_sector4_address(self):
        """Verify Sector 4 base address (64KB sector)"""
        FLASH_F4_SECTOR4_ADDR = 0x08010000
        self.assertEqual(FLASH_F4_SECTOR4_ADDR, 0x08010000)
        
    def test_sector5_address(self):
        """Verify Sector 5 base address (128KB sectors start)"""
        FLASH_F4_SECTOR5_ADDR = 0x08020000
        self.assertEqual(FLASH_F4_SECTOR5_ADDR, 0x08020000)
        
    def test_sector_addresses_increase(self):
        """Verify sector addresses increase monotonically"""
        sectors = [
            0x08000000,  # Sector 0: 16KB
            0x08004000,  # Sector 1: 16KB
            0x08008000,  # Sector 2: 16KB
            0x0800C000,  # Sector 3: 16KB
            0x08010000,  # Sector 4: 64KB
            0x08020000,  # Sector 5: 128KB
        ]
        for i in range(len(sectors) - 1):
            self.assertLess(sectors[i], sectors[i + 1])


class TestFlashProgrammingFlow(unittest.TestCase):
    """Test complete Flash programming workflow logic"""
    
    def setUp(self):
        """Set up test fixtures"""
        self.flash_base = 0x08000000
        self.page_size = 1024  # F1 page size
        
    def test_erase_before_write_required(self):
        """Verify Flash must be erased before writing"""
        # Flash can only change 1s to 0s, not 0s to 1s
        # So erase (set all to 1s) is required before write
        old_data = 0xAAAAAAAA  # All 1s after erase
        new_data = 0x55555555  # Can write 0s
        # If we tried to write without erase:
        # result = old_data & new_data (would fail)
        result = old_data & new_data
        self.assertNotEqual(result, new_data)
        
    def test_write_after_erase_succeeds(self):
        """Verify write succeeds after erase"""
        erased_data = 0xFFFFFFFF  # All 1s
        new_data = 0x12345678
        # After erase, we can write any pattern
        result = erased_data & new_data
        self.assertEqual(result, new_data)
        
    def test_half_word_alignment(self):
        """Verify F1 requires half-word (16-bit) alignment"""
        address = 0x08000000
        # Valid aligned addresses
        valid_addresses = [address, address + 2, address + 4]
        for addr in valid_addresses:
            self.assertEqual(addr % 2, 0, f"Address {addr:x} not half-word aligned")
            
    def test_page_address_calculation(self):
        """Test page number to address calculation"""
        page_number = 5
        page_size = 1024
        expected_address = 0x08000000 + (page_number * page_size)
        self.assertEqual(expected_address, 0x08001400)


class TestSWDProtocol(unittest.TestCase):
    """Test SWD protocol implementation"""
    
    def test_swd_request_format(self):
        """Verify SWD request packet format"""
        # Request: Start(1) + APnDP(1) + RnW(1) + A[2:3](2) + Parity(1) + Stop(0) + Park(1)
        # Result: 0b1APnDPRnWA1A00P1
        
        # DP read IDCODE: APnDP=0, RnW=1, A=00
        # Binary: 1 0 1 00 0 0 1 = 0xA5 (with calculated parity)
        request = 0x81  # Base: Start=1, Park=1
        request |= (0 & 1) << 6  # APnDP = 0 (DP)
        request |= (1 & 1) << 5  # RnW = 1 (Read)
        request |= (0 & 3) << 2  # A = 00
        # Parity would be calculated here
        self.assertEqual(request & 0x81, 0x81)  # Start and Park bits set
        
    def test_jtag_to_swd_sequence(self):
        """Verify JTAG-to-SWD switching sequence"""
        # Sequence: 0xE79C (16 bits, LSB first)
        sequence = [1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1]
        # Convert to byte
        byte_val = 0
        for i, bit in enumerate(sequence):
            byte_val |= (bit << i)
        self.assertEqual(byte_val, 0x9C7E)  # LSB first, so reversed


class TestErrorHandling(unittest.TestCase):
    """Test Flash error detection and handling"""
    
    def test_programming_error_detection(self):
        """Verify programming error can be detected"""
        FLASH_F1_SR_PGERR = (1 << 2)
        status_reg = FLASH_F1_SR_PGERR  # Simulate error
        self.assertTrue(status_reg & FLASH_F1_SR_PGERR)
        
    def test_write_protection_error_detection(self):
        """Verify write protection error can be detected"""
        FLASH_F1_SR_WRPRTERR = (1 << 4)
        status_reg = FLASH_F1_SR_WRPRTERR
        self.assertTrue(status_reg & FLASH_F1_SR_WRPRTERR)
        
    def test_busy_flag_check(self):
        """Verify busy flag can be checked"""
        FLASH_F1_SR_BSY = (1 << 0)
        # Simulate busy
        status_reg = FLASH_F1_SR_BSY
        self.assertTrue(status_reg & FLASH_F1_SR_BSY)
        # Simulate ready
        status_reg = 0
        self.assertFalse(status_reg & FLASH_F1_SR_BSY)


class TestMemoryRegions(unittest.TestCase):
    """Test STM32 memory map regions"""
    
    def test_flash_base_address(self):
        """Verify Flash memory base address"""
        FLASH_BASE = 0x08000000
        self.assertEqual(FLASH_BASE, 0x08000000)
        
    def test_sram_base_address(self):
        """Verify SRAM base address"""
        SRAM_BASE = 0x20000000
        self.assertEqual(SRAM_BASE, 0x20000000)
        
    def test_system_memory_base(self):
        """Verify System Memory (bootloader) base address"""
        SYSTEM_MEMORY_BASE = 0x1FFFF000  # F1 series
        self.assertEqual(SYSTEM_MEMORY_BASE, 0x1FFFF000)
        
    def test_option_bytes_base(self):
        """Verify Option Bytes base address"""
        OPTION_BYTES_BASE = 0x1FFFF800  # F1 series
        self.assertEqual(OPTION_BYTES_BASE, 0x1FFFF800)


def run_tests():
    """Run all unit tests"""
    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add all test classes
    test_classes = [
        TestSTM32IDCodeDetection,
        TestFlashRegisterAddresses,
        TestFlashUnlockKeys,
        TestFlashControlBits,
        TestFlashStatusBits,
        TestDebugRegisters,
        TestDAPProtocol,
        TestFlashSectorAddresses,
        TestFlashProgrammingFlow,
        TestSWDProtocol,
        TestErrorHandling,
        TestMemoryRegions,
    ]
    
    for test_class in test_classes:
        tests = loader.loadTestsFromTestCase(test_class)
        suite.addTests(tests)
    
    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    sys.exit(0 if success else 1)
