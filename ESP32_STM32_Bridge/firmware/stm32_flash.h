/**
 * STM32 Flash Programming Algorithms
 * 
 * Supports: STM32F1xx, STM32F4xx, STM32F7xx, STM32H7xx series
 * 
 * Based on STM32 reference manuals:
 * - RM0008: STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx, STM32F107xx
 * - RM0090: STM32F405/415, STM32F407/417, STM32F427/437, STM32F429/439
 * - RM0410: STM32F76xxx and STM32F77xxx
 * - RM0433: STM32H7xx
 */

#ifndef STM32_FLASH_H
#define STM32_FLASH_H

#include <Arduino.h>
#include <stdint.h>

// ============== MCU Family Detection ==============
typedef enum {
  STM32_FAMILY_UNKNOWN = 0,
  STM32_FAMILY_F1,
  STM32_FAMILY_F4,
  STM32_FAMILY_F7,
  STM32_FAMILY_H7,
  STM32_FAMILY_L0,
  STM32_FAMILY_L4,
  STM32_FAMILY_G0,
  STM32_FAMILY_WB
} STM32_Family;

typedef struct {
  uint32_t idcode;
  uint16_t flash_size_kb;
  uint16_t page_size;        // For F1: 1KB, F4: variable sectors
  uint16_t ram_size_kb;
  STM32_Family family;
  const char* name;
} MCU_Info;

// IDCODE to family mapping (from ARM CoreSight IDR)
#define IDCODE_STM32F1_MASK     0xFFF
#define IDCODE_STM32F1_VALUE    0x410  // Cortex-M3 r1p1

#define IDCODE_STM32F4_MASK     0xFFF
#define IDCODE_STM32F4_VALUE    0x413  // Cortex-M4 r0p1

#define IDCODE_STM32F7_MASK     0xFFF
#define IDCODE_STM32F7_VALUE    0x451  // Cortex-M7 r0p1

#define IDCODE_STM32H7_MASK     0xFFF
#define IDCODE_STM32H7_VALUE    0x450  // Cortex-M7 r0p0

// ============== STM32F1xx Flash Registers ==============
// Flash interface registers base address
#define FLASH_F1_BASE           0x40022000

#define FLASH_F1_ACR            (FLASH_F1_BASE + 0x00)  // Access control
#define FLASH_F1_KEYR           (FLASH_F1_BASE + 0x04)  // Key register
#define FLASH_F1_OPTKEYR        (FLASH_F1_BASE + 0x08)  // Option key
#define FLASH_F1_SR             (FLASH_F1_BASE + 0x0C)  // Status register
#define FLASH_F1_CR             (FLASH_F1_BASE + 0x10)  // Control register
#define FLASH_F1_AR             (FLASH_F1_BASE + 0x14)  // Address register
#define FLASH_F1_OBR            (FLASH_F1_BASE + 0x1C)  // Option byte
#define FLASH_F1_WRPR           (FLASH_F1_BASE + 0x20)  // Write protect

// FLASH_F1_CR bits
#define FLASH_F1_CR_PG          (1 << 0)   // Programming
#define FLASH_F1_CR_PER         (1 << 1)   // Page erase
#define FLASH_F1_CR_MER         (1 << 2)   // Mass erase
#define FLASH_F1_CR_OPTPG       (1 << 4)   // Option byte programming
#define FLASH_F1_CR_OPTER       (1 << 5)   // Option byte erase
#define FLASH_F1_CR_STRT        (1 << 6)   // Start
#define FLASH_F1_CR_LOCK        (1 << 7)   // Lock
#define FLASH_F1_CR_OPTWRE      (1 << 9)   // Option byte write enable
#define FLASH_F1_CR_ERRIE       (1 << 10)  // Error interrupt
#define FLASH_F1_CR_EOPIE       (1 << 12)  // End of operation interrupt

// FLASH_F1_SR bits
#define FLASH_F1_SR_BSY         (1 << 0)   // Busy
#define FLASH_F1_SR_PGERR       (1 << 2)   // Programming error
#define FLASH_F1_SR_WRPRTERR    (1 << 4)   // Write protection error
#define FLASH_F1_SR_EOP         (1 << 5)   // End of operation

// Unlock keys
#define FLASH_F1_KEY1           0x45670123
#define FLASH_F1_KEY2           0xCDEF89AB

// ============== STM32F4xx Flash Registers ==============
#define FLASH_F4_BASE           0x40023C00

#define FLASH_F4_ACR            (FLASH_F4_BASE + 0x00)
#define FLASH_F4_KEYR           (FLASH_F4_BASE + 0x04)
#define FLASH_F4_OPTKEYR        (FLASH_F4_BASE + 0x08)
#define FLASH_F4_SR             (FLASH_F4_BASE + 0x0C)
#define FLASH_F4_CR             (FLASH_F4_BASE + 0x10)
#define FLASH_F4_OPTCR          (FLASH_F4_BASE + 0x14)

// FLASH_F4_CR bits (RM0090 Section 3.9.5)
#define FLASH_F4_CR_PG          (1 << 0)    // Programming
#define FLASH_F4_CR_SER         (1 << 1)    // Sector erase
#define FLASH_F4_CR_MER         (1 << 2)    // Mass erase (bank 1)
#define FLASH_F4_CR_MER2        (1 << 15)   // Mass erase (bank 2, for 2MB devices)
#define FLASH_F4_CR_SNB_SHIFT   3           // Sector number shift
#define FLASH_F4_CR_SNB_MASK    (0xF << 3)  // Sector number mask
#define FLASH_F4_CR_PSIZE_SHIFT 8           // Program size shift
#define FLASH_F4_CR_PSIZE_MASK  (0x3 << 8)  // Program size mask
#define FLASH_F4_CR_PSIZE_8     (0 << 8)    // 8-bit programming
#define FLASH_F4_CR_PSIZE_16    (1 << 8)    // 16-bit programming
#define FLASH_F4_CR_PSIZE_32    (2 << 8)    // 32-bit programming
#define FLASH_F4_CR_PSIZE_64    (3 << 8)    // 64-bit programming (requires 2.7-3.6V)
#define FLASH_F4_CR_STRT        (1 << 16)   // Start operation
#define FLASH_F4_CR_EOPIE       (1 << 24)   // End of operation interrupt enable
#define FLASH_F4_CR_ERRIE       (1 << 25)   // Error interrupt enable
#define FLASH_F4_CR_LOCK        (1 << 31)   // Lock

// FLASH_F4_SR bits (RM0090 Section 3.9.4)
#define FLASH_F4_SR_EOP         (1 << 0)    // End of operation
#define FLASH_F4_SR_SOP         (1 << 1)    // Operation error
#define FLASH_F4_SR_WRPERR      (1 << 4)    // Write protection error
#define FLASH_F4_SR_PGAERR      (1 << 5)    // Programming alignment error
#define FLASH_F4_SR_PGPERR      (1 << 6)    // Programming parallelism error
#define FLASH_F4_SR_PGSERR      (1 << 7)    // Programming sequence error
#define FLASH_F4_SR_BSY         (1 << 16)   // Busy

// Unlock keys (same as F1)
#define FLASH_F4_KEY1           0x45670123
#define FLASH_F4_KEY2           0xCDEF89AB

// Sector addresses for F4 (1MB dual bank)
#define FLASH_F4_SECTOR0_ADDR   0x08000000  // 16 KB
#define FLASH_F4_SECTOR1_ADDR   0x08004000  // 16 KB
#define FLASH_F4_SECTOR2_ADDR   0x08008000  // 16 KB
#define FLASH_F4_SECTOR3_ADDR   0x0800C000  // 16 KB
#define FLASH_F4_SECTOR4_ADDR   0x08010000  // 64 KB
#define FLASH_F4_SECTOR5_ADDR   0x08020000  // 128 KB
#define FLASH_F4_SECTOR6_ADDR   0x08040000  // 128 KB
#define FLASH_F4_SECTOR7_ADDR   0x08060000  // 128 KB

// Sector sizes (in bytes)
#define FLASH_F4_SECTOR_SIZE_0  (16 * 1024)   // 16 KB
#define FLASH_F4_SECTOR_SIZE_1  (16 * 1024)   // 16 KB
#define FLASH_F4_SECTOR_SIZE_2  (16 * 1024)   // 16 KB
#define FLASH_F4_SECTOR_SIZE_3  (16 * 1024)   // 16 KB
#define FLASH_F4_SECTOR_SIZE_4  (64 * 1024)   // 64 KB
#define FLASH_F4_SECTOR_SIZE_5  (128 * 1024)  // 128 KB
#define FLASH_F4_SECTOR_SIZE_6  (128 * 1024)  // 128 KB
#define FLASH_F4_SECTOR_SIZE_7  (128 * 1024)  // 128 KB

// ============== Core Debug Registers ==============
#define DEBUG_DHCSR             0xE000EDF0  // Debug Halting Control and Status
#define DEBUG_DCRSR             0xE000EDF4  // Debug Core Register Selector
#define DEBUG_DCRDR             0xE000EDF8  // Debug Core Register Data
#define DEBUG_DEMCR             0xE000EDFC  // Debug Exception and Monitor Control

#define DHCSR_DBGKEY            0xA05F0000
#define DHCSR_C_DEBUGEN         (1 << 0)
#define DHCSR_C_HALT            (1 << 1)
#define DHCSR_C_STEP            (1 << 2)
#define DHCSR_C_MASKINTS        (1 << 3)
#define DHCSR_S_REGRDY          (1 << 16)
#define DHCSR_S_HALT            (1 << 17)
#define DHCSR_S_SLEEP           (1 << 18)
#define DHCSR_S_LOCKUP          (1 << 19)
#define DHCSR_S_RETIRE_ST       (1 << 24)
#define DHCSR_S_RESET_ST        (1 << 25)

#define DCRSR_REGWnR            (1 << 16)

// ============== DAP Register Addresses ==============
#define DAP_DP_SELECT           0x08
#define DAP_DP_RDBUFF           0x0C

#define DAP_AP_CSW              0x00
#define DAP_AP_TAR              0x04
#define DAP_AP_DRW              0x0C

#define AP_CSW_SIZE_BYTE        0x00000000
#define AP_CSW_SIZE_HALF        0x00000001
#define AP_CSW_SIZE_WORD        0x00000002
#define AP_CSW_ADDRINC_OFF      0x00000000
#define AP_CSW_ADDRINC_SINGLE   0x00000010
#define AP_CSW_ADDRINC_PACKED   0x00000020
#define AP_CSW_DEVICEEN         0x00000040
#define AP_CSW_TRINPROG         0x00000080
#define AP_CSW_DBGSWENABLE      0x80000000

// ============== SWD External Functions ==============
// These are defined in the main firmware
extern uint8_t swdTransfer(uint8_t request, uint32_t* data);
extern uint8_t swdRequest(uint8_t apndp, uint8_t rnw, uint8_t addr);
extern void swdReset(void);
extern void swdWriteBit(uint8_t bit);
extern uint8_t swdReadBit(void);

// ============== Flash Programming Class ==============
class STM32FlashProgrammer {
public:
  STM32FlashProgrammer();
  
  // Initialize and detect MCU
  bool init(void);
  bool detectMCU(uint32_t idcode);
  
  // Core control
  bool haltCore(void);
  bool resetCore(void);
  bool resetSystem(void);
  
  // Flash operations
  bool unlockFlash(void);
  bool lockFlash(void);
  bool eraseAll(void);
  bool erasePage(uint32_t address);
  bool eraseSector(uint32_t sector_num);
  bool writeHalfWord(uint32_t address, uint16_t data);
  bool writeWord(uint32_t address, uint32_t data);
  bool writeBuffer(uint32_t address, uint8_t* data, uint32_t size);
  bool verifyBuffer(uint32_t address, uint8_t* data, uint32_t size);
  
  // Getters
  STM32_Family getFamily(void) { return mcu_info.family; }
  const char* getMCUName(void) { return mcu_info.name; }
  uint32_t getFlashSize(void) { return mcu_info.flash_size_kb * 1024; }
  
private:
  MCU_Info mcu_info;
  bool flash_unlocked;
  
  // Register access
  bool writeMem(uint32_t address, uint32_t data);
  bool readMem(uint32_t address, uint32_t* data);
  bool writeAP(uint8_t reg, uint32_t data);
  bool readAP(uint8_t reg, uint32_t* data);
  bool writeDP(uint8_t reg, uint32_t data);
  bool readDP(uint8_t reg, uint32_t* data);
  
  // Family-specific implementations
  bool unlockFlash_F1(void);
  bool eraseAll_F1(void);
  bool erasePage_F1(uint32_t address);
  bool writeHalfWord_F1(uint32_t address, uint16_t data);
  bool waitForFlash_F1(uint32_t timeout_ms);
  
  bool unlockFlash_F4(void);
  bool eraseAll_F4(void);
  bool eraseSector_F4(uint32_t sector);
  bool writeWord_F4(uint32_t address, uint32_t data);
  bool waitForFlash_F4(uint32_t timeout_ms);
};

// ============== Implementation ==============

STM32FlashProgrammer::STM32FlashProgrammer() 
  : flash_unlocked(false) {
  mcu_info.idcode = 0;
  mcu_info.flash_size_kb = 0;
  mcu_info.page_size = 1024;
  mcu_info.ram_size_kb = 0;
  mcu_info.family = STM32_FAMILY_UNKNOWN;
  mcu_info.name = "Unknown";
}

bool STM32FlashProgrammer::writeDP(uint8_t reg, uint32_t data) {
  uint8_t request = swdRequest(0, 0, reg);  // DP, Write
  if (swdTransfer(request, &data) != 0) return false;
  return true;
}

bool STM32FlashProgrammer::readDP(uint8_t reg, uint32_t* data) {
  uint8_t request = swdRequest(0, 1, reg);  // DP, Read
  if (swdTransfer(request, data) != 0) return false;
  
  // RDBUFF read required for valid data
  if (reg != DAP_DP_RDBUFF) {
    request = swdRequest(0, 1, DAP_DP_RDBUFF);
    if (swdTransfer(request, data) != 0) return false;
  }
  return true;
}

bool STM32FlashProgrammer::writeAP(uint8_t reg, uint32_t data) {
  // Select AP bank
  if (!writeDP(DAP_DP_SELECT, 0x00000000)) return false;
  
  uint8_t request = swdRequest(1, 0, reg);  // AP, Write
  if (swdTransfer(request, &data) != 0) return false;
  return true;
}

bool STM32FlashProgrammer::readAP(uint8_t reg, uint32_t* data) {
  // Select AP bank
  if (!writeDP(DAP_DP_SELECT, 0x00000000)) return false;
  
  uint8_t request = swdRequest(1, 1, reg);  // AP, Read
  if (swdTransfer(request, data) != 0) return false;
  
  // RDBUFF read required for valid data
  request = swdRequest(0, 1, DAP_DP_RDBUFF);
  if (swdTransfer(request, data) != 0) return false;
  return true;
}

bool STM32FlashProgrammer::writeMem(uint32_t address, uint32_t data) {
  // Configure AP for word access
  if (!writeAP(DAP_AP_CSW, AP_CSW_SIZE_WORD | AP_CSW_ADDRINC_OFF | 
               AP_CSW_DEVICEEN | AP_CSW_DBGSWENABLE)) {
    return false;
  }
  
  // Set transfer address
  if (!writeAP(DAP_AP_TAR, address)) return false;
  
  // Write data
  if (!writeAP(DAP_AP_DRW, data)) return false;
  
  return true;
}

bool STM32FlashProgrammer::readMem(uint32_t address, uint32_t* data) {
  // Configure AP for word access
  if (!writeAP(DAP_AP_CSW, AP_CSW_SIZE_WORD | AP_CSW_ADDRINC_OFF | 
               AP_CSW_DEVICEEN | AP_CSW_DBGSWENABLE)) {
    return false;
  }
  
  // Set transfer address
  if (!writeAP(DAP_AP_TAR, address)) return false;
  
  // Read data
  if (!readAP(DAP_AP_DRW, data)) return false;
  
  return true;
}

bool STM32FlashProgrammer::detectMCU(uint32_t idcode) {
  mcu_info.idcode = idcode;
  
  // Detect family from IDCODE
  uint16_t part_no = (idcode >> 12) & 0xFFF;
  
  switch (part_no) {
    case IDCODE_STM32F1_VALUE:
      mcu_info.family = STM32_FAMILY_F1;
      mcu_info.name = "STM32F1xx";
      mcu_info.page_size = 1024;  // 1KB pages
      mcu_info.flash_size_kb = 128;  // Default, should read from device
      break;
      
    case IDCODE_STM32F4_VALUE:
      mcu_info.family = STM32_FAMILY_F4;
      mcu_info.name = "STM32F4xx";
      mcu_info.page_size = 0;  // Variable sectors
      mcu_info.flash_size_kb = 1024;
      break;
      
    case IDCODE_STM32F7_VALUE:
      mcu_info.family = STM32_FAMILY_F7;
      mcu_info.name = "STM32F7xx";
      mcu_info.page_size = 0;
      mcu_info.flash_size_kb = 512;
      break;
      
    case IDCODE_STM32H7_VALUE:
      mcu_info.family = STM32_FAMILY_H7;
      mcu_info.name = "STM32H7xx";
      mcu_info.page_size = 0;
      mcu_info.flash_size_kb = 2048;
      break;
      
    default:
      mcu_info.family = STM32_FAMILY_UNKNOWN;
      mcu_info.name = "Unknown MCU";
      return false;
  }
  
  return true;
}

bool STM32FlashProgrammer::init(void) {
  swdReset();
  
  uint32_t idcode;
  if (!readDP(DP_IDCODE, &idcode)) {
    return false;
  }
  
  if (!detectMCU(idcode)) {
    return false;
  }
  
  return true;
}

bool STM32FlashProgrammer::haltCore(void) {
  uint32_t dhcsr;
  
  // Read DHCSR
  if (!readMem(DEBUG_DHCSR, &dhcsr)) return false;
  
  // Write debug key + enable debug + halt
  dhcsr = DHCSR_DBGKEY | DHCSR_C_DEBUGEN | DHCSR_C_HALT;
  if (!writeMem(DEBUG_DHCSR, dhcsr)) return false;
  
  // Wait for halt
  uint32_t timeout = millis() + 1000;
  while (millis() < timeout) {
    if (!readMem(DEBUG_DHCSR, &dhcsr)) return false;
    if (dhcsr & DHCSR_S_HALT) return true;
    delay(1);
  }
  
  return false;
}

bool STM32FlashProgrammer::resetSystem(void) {
  uint32_t demcr;
  
  // Enable reset vector catch
  if (!readMem(DEBUG_DEMCR, &demcr)) return false;
  demcr |= (1 << 0);  // VC_CORERESET
  if (!writeMem(DEBUG_DEMCR, demcr)) return false;
  
  // Request system reset
  uint32_t aircr = 0x05FA0004;  // VECTKEY + SYSRESETREQ
  if (!writeMem(0xE000ED0C, aircr)) return false;
  
  delay(100);
  
  // Wait for reset to complete
  uint32_t dhcsr;
  uint32_t timeout = millis() + 2000;
  while (millis() < timeout) {
    if (!readMem(DEBUG_DHCSR, &dhcsr)) continue;
    if (dhcsr & DHCSR_S_RESET_ST) {
      // Clear reset vector catch
      demcr &= ~(1 << 0);
      writeMem(DEBUG_DEMCR, demcr);
      return true;
    }
    delay(10);
  }
  
  return false;
}

// ============== STM32F1xx Specific Implementation ==============

bool STM32FlashProgrammer::unlockFlash_F1(void) {
  uint32_t cr;
  
  // Read current CR
  if (!readMem(FLASH_F1_CR, &cr)) return false;
  
  // Already unlocked?
  if (!(cr & FLASH_F1_CR_LOCK)) {
    flash_unlocked = true;
    return true;
  }
  
  // Write KEY1
  if (!writeMem(FLASH_F1_KEYR, FLASH_F1_KEY1)) return false;
  
  // Write KEY2
  if (!writeMem(FLASH_F1_KEYR, FLASH_F1_KEY2)) return false;
  
  // Verify unlocked
  if (!readMem(FLASH_F1_CR, &cr)) return false;
  if (cr & FLASH_F1_CR_LOCK) return false;
  
  flash_unlocked = true;
  return true;
}

bool STM32FlashProgrammer::waitForFlash_F1(uint32_t timeout_ms) {
  uint32_t sr;
  uint32_t timeout = millis() + timeout_ms;
  
  while (millis() < timeout) {
    if (!readMem(FLASH_F1_SR, &sr)) return false;
    
    // Check for errors
    if (sr & (FLASH_F1_SR_PGERR | FLASH_F1_SR_WRPRTERR)) {
      return false;
    }
    
    // Check if busy bit is clear
    if (!(sr & FLASH_F1_SR_BSY)) {
      return true;
    }
    
    delay(1);
  }
  
  return false;  // Timeout
}

bool STM32FlashProgrammer::erasePage_F1(uint32_t address) {
  if (!flash_unlocked) {
    if (!unlockFlash_F1()) return false;
  }
  
  uint32_t cr;
  
  // Set page erase bit
  if (!readMem(FLASH_F1_CR, &cr)) return false;
  cr |= FLASH_F1_CR_PER;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  // Set page address
  if (!writeMem(FLASH_F1_AR, address)) return false;
  
  // Start erase
  cr |= FLASH_F1_CR_STRT;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  // Wait for completion
  if (!waitForFlash_F1(5000)) return false;
  
  // Clear page erase bit
  cr &= ~FLASH_F1_CR_PER;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  return true;
}

bool STM32FlashProgrammer::eraseAll_F1(void) {
  if (!flash_unlocked) {
    if (!unlockFlash_F1()) return false;
  }
  
  uint32_t cr;
  
  // Set mass erase bit
  if (!readMem(FLASH_F1_CR, &cr)) return false;
  cr |= FLASH_F1_CR_MER;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  // Start erase
  cr |= FLASH_F1_CR_STRT;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  // Wait for completion (mass erase takes longer)
  if (!waitForFlash_F1(40000)) return false;
  
  // Clear mass erase bit
  cr &= ~FLASH_F1_CR_MER;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  return true;
}

bool STM32FlashProgrammer::writeHalfWord_F1(uint32_t address, uint16_t data) {
  if (!flash_unlocked) {
    if (!unlockFlash_F1()) return false;
  }
  
  // Wait for not busy
  if (!waitForFlash_F1(100)) return false;
  
  uint32_t cr;
  
  // Set programming bit
  if (!readMem(FLASH_F1_CR, &cr)) return false;
  cr |= FLASH_F1_CR_PG;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  // Write half-word (little-endian)
  uint32_t word_data = data;
  if (!writeMem(address, word_data)) return false;
  
  // Wait for completion
  if (!waitForFlash_F1(100)) return false;
  
  // Clear programming bit
  cr &= ~FLASH_F1_CR_PG;
  if (!writeMem(FLASH_F1_CR, cr)) return false;
  
  return true;
}

// ============== STM32F4xx Specific Implementation ==============

bool STM32FlashProgrammer::unlockFlash_F4(void) {
  uint32_t cr;
  
  if (!readMem(FLASH_F4_CR, &cr)) return false;
  
  if (!(cr & FLASH_F4_CR_LOCK)) {
    flash_unlocked = true;
    return true;
  }
  
  if (!writeMem(FLASH_F4_KEYR, FLASH_F4_KEY1)) return false;
  if (!writeMem(FLASH_F4_KEYR, FLASH_F4_KEY2)) return false;
  
  if (!readMem(FLASH_F4_CR, &cr)) return false;
  if (cr & FLASH_F4_CR_LOCK) return false;
  
  flash_unlocked = true;
  return true;
}

bool STM32FlashProgrammer::waitForFlash_F4(uint32_t timeout_ms) {
  uint32_t sr;
  uint32_t timeout = millis() + timeout_ms;
  
  while (millis() < timeout) {
    if (!readMem(FLASH_F4_SR, &sr)) return false;
    
    if (sr & (FLASH_F4_SR_SOP | FLASH_F4_SR_WRPERR | 
              FLASH_F4_SR_PGAERR | FLASH_F4_SR_PGPERR | FLASH_F4_SR_PGSERR)) {
      return false;
    }
    
    if (!(sr & FLASH_F4_SR_BSY)) {
      return true;
    }
    
    delay(1);
  }
  
  return false;
}

bool STM32FlashProgrammer::eraseSector_F4(uint32_t sector_num) {
  if (!flash_unlocked && !unlockFlash_F4()) return false;
  
  if (sector_num > 11) return false;
  
  uint32_t cr;
  
  if (!writeMem(FLASH_F4_SR, 0x000000F3)) return false;
  
  if (!readMem(FLASH_F4_CR, &cr)) return false;
  cr &= ~FLASH_F4_CR_SNB_MASK;
  cr |= FLASH_F4_CR_SER | (sector_num << FLASH_F4_CR_SNB_SHIFT);
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  cr |= FLASH_F4_CR_STRT;
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  if (!waitForFlash_F4(8000)) return false;
  
  cr &= ~(FLASH_F4_CR_SER | FLASH_F4_CR_SNB_MASK);
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  return true;
}

bool STM32FlashProgrammer::eraseAll_F4(void) {
  if (!flash_unlocked && !unlockFlash_F4()) return false;
  
  uint32_t cr;
  
  if (!writeMem(FLASH_F4_SR, 0x000000F3)) return false;
  
  if (!readMem(FLASH_F4_CR, &cr)) return false;
  cr |= FLASH_F4_CR_MER;
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  cr |= FLASH_F4_CR_STRT;
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  if (!waitForFlash_F4(30000)) return false;
  
  cr &= ~FLASH_F4_CR_MER;
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  return true;
}

bool STM32FlashProgrammer::writeWord_F4(uint32_t address, uint32_t data) {
  if (!flash_unlocked && !unlockFlash_F4()) return false;
  
  if (!waitForFlash_F4(100)) return false;
  
  uint32_t cr;
  
  if (!writeMem(FLASH_F4_SR, 0x000000F3)) return false;
  
  if (!readMem(FLASH_F4_CR, &cr)) return false;
  cr &= ~FLASH_F4_CR_PSIZE_MASK;
  cr |= FLASH_F4_CR_PG | FLASH_F4_CR_PSIZE_32;
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  if (!writeMem(address, data)) return false;
  
  if (!waitForFlash_F4(100)) return false;
  
  cr &= ~(FLASH_F4_CR_PG | FLASH_F4_CR_PSIZE_MASK);
  if (!writeMem(FLASH_F4_CR, cr)) return false;
  
  return true;
}

// ============== Generic API Implementation ==============

bool STM32FlashProgrammer::unlockFlash(void) {
  if (mcu_info.family == STM32_FAMILY_UNKNOWN) {
    return false;
  }
  
  switch (mcu_info.family) {
    case STM32_FAMILY_F1:
      return unlockFlash_F1();
      
    case STM32_FAMILY_F4:
      return unlockFlash_F4();
      
    default:
      return false;
  }
}

bool STM32FlashProgrammer::lockFlash(void) {
  if (mcu_info.family == STM32_FAMILY_UNKNOWN) {
    return false;
  }
  
  uint32_t cr;
  
  switch (mcu_info.family) {
    case STM32_FAMILY_F1:
      if (!readMem(FLASH_F1_CR, &cr)) return false;
      cr |= FLASH_F1_CR_LOCK;
      if (!writeMem(FLASH_F1_CR, cr)) return false;
      break;
      
    case STM32_FAMILY_F4:
      if (!readMem(FLASH_F4_CR, &cr)) return false;
      cr |= FLASH_F4_CR_LOCK;
      if (!writeMem(FLASH_F4_CR, cr)) return false;
      break;
      
    default:
      return false;
  }
  
  flash_unlocked = false;
  return true;
}

bool STM32FlashProgrammer::eraseAll(void) {
  if (mcu_info.family == STM32_FAMILY_UNKNOWN) {
    return false;
  }
  
  if (!haltCore()) return false;
  
  switch (mcu_info.family) {
    case STM32_FAMILY_F1:
      return eraseAll_F1();
      
    case STM32_FAMILY_F4:
      return eraseAll_F4();
      
    default:
      return false;
  }
}

bool STM32FlashProgrammer::erasePage(uint32_t address) {
  if (mcu_info.family == STM32_FAMILY_UNKNOWN) {
    return false;
  }
  
  if (!haltCore()) return false;
  
  switch (mcu_info.family) {
    case STM32_FAMILY_F1:
      return erasePage_F1(address);
      
    case STM32_FAMILY_F4: {
      uint32_t sector;
      if (address < FLASH_F4_SECTOR4_ADDR) {
        sector = (address - FLASH_F4_SECTOR0_ADDR) / FLASH_F4_SECTOR_SIZE_0;
      } else if (address < FLASH_F4_SECTOR5_ADDR) {
        sector = 4;
      } else {
        sector = 5 + (address - FLASH_F4_SECTOR5_ADDR) / FLASH_F4_SECTOR_SIZE_5;
      }
      return eraseSector_F4(sector);
    }
      
    default:
      return false;
  }
}

bool STM32FlashProgrammer::writeBuffer(uint32_t address, uint8_t* data, uint32_t size) {
  if (mcu_info.family == STM32_FAMILY_UNKNOWN) {
    return false;
  }
  
  if (!haltCore()) return false;
  
  if (!flash_unlocked && !unlockFlash()) return false;
  
  uint32_t offset = 0;
  
  while (offset < size) {
    bool success = false;
    
    switch (mcu_info.family) {
      case STM32_FAMILY_F1:
        if (offset + 1 < size) {
          uint16_t halfword = data[offset] | (data[offset + 1] << 8);
          success = writeHalfWord_F1(address + offset, halfword);
          offset += 2;
        } else {
          uint16_t halfword = data[offset] | 0xFF00;
          success = writeHalfWord_F1(address + offset, halfword);
          offset += 1;
        }
        break;
        
      case STM32_FAMILY_F4:
        if (offset + 3 < size) {
          uint32_t word = data[offset] | (data[offset + 1] << 8) | 
                          (data[offset + 2] << 16) | (data[offset + 3] << 24);
          success = writeWord_F4(address + offset, word);
          offset += 4;
        } else {
          uint32_t word = 0xFFFFFFFF;
          for (int i = 0; i < 4 && (offset + i) < size; i++) {
            word &= ~(0xFF << (i * 8));
            word |= data[offset + i] << (i * 8);
          }
          success = writeWord_F4(address + offset, word);
          offset += (size - offset);
        }
        break;
        
      default:
        return false;
    }
    
    if (!success) {
      return false;
    }
  }
  
  return true;
}

bool STM32FlashProgrammer::verifyBuffer(uint32_t address, uint8_t* data, uint32_t size) {
  if (mcu_info.family == STM32_FAMILY_UNKNOWN) {
    return false;
  }
  
  for (uint32_t offset = 0; offset < size; offset += 4) {
    uint32_t word;
    if (!readMem(address + offset, &word)) {
      return false;
    }
    
    // Compare bytes
    for (int i = 0; i < 4 && (offset + i) < size; i++) {
      uint8_t expected = data[offset + i];
      uint8_t actual = (word >> (i * 8)) & 0xFF;
      if (expected != actual) {
        return false;
      }
    }
  }
  
  return true;
}

#endif // STM32_FLASH_H
