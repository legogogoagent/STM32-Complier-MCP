"""STM32 MCU数据库 - 支持多目标系列的设备信息

包含STM32F0/F1/F2/F3/F4/F7/H7等系列的设备IDCODE映射和配置信息
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Any
from enum import Enum


class MCUFamily(str, Enum):
    """STM32 MCU系列枚举"""
    F0 = "STM32F0"
    F1 = "STM32F1"
    F2 = "STM32F2"
    F3 = "STM32F3"
    F4 = "STM32F4"
    F7 = "STM32F7"
    H7 = "STM32H7"
    L0 = "STM32L0"
    L1 = "STM32L1"
    L4 = "STM32L4"
    L5 = "STM32L5"
    G0 = "STM32G0"
    G4 = "STM32G4"
    U5 = "STM32U5"
    WB = "STM32WB"
    WL = "STM32WL"


@dataclass
class MCUInfo:
    """MCU信息数据类"""
    name: str
    family: MCUFamily
    device_id: str  # IDCODE, e.g., "0x20036410"
    flash_size_kb: int
    ram_size_kb: int
    core: str  # Cortex-M0, Cortex-M3, etc.
    target_config: str  # OpenOCD target config filename
    description: str = ""
    max_clock_mhz: int = 72
    has_dual_bank: bool = False
    

# STM32 MCU数据库 - 按系列组织的设备信息
STM32_MCU_DATABASE: Dict[str, MCUInfo] = {
    # ========== STM32F1系列 ==========
    "0x20036410": MCUInfo(
        name="STM32F103C8",
        family=MCUFamily.F1,
        device_id="0x20036410",
        flash_size_kb=64,
        ram_size_kb=20,
        core="Cortex-M3",
        target_config="stm32f1x.cfg",
        description="STM32F103C8T6 - Medium-density",
        max_clock_mhz=72
    ),
    "0x20036414": MCUInfo(
        name="STM32F103CB",
        family=MCUFamily.F1,
        device_id="0x20036414",
        flash_size_kb=128,
        ram_size_kb=20,
        core="Cortex-M3",
        target_config="stm32f1x.cfg",
        description="STM32F103CBT6 - Medium-density",
        max_clock_mhz=72
    ),
    "0x20036420": MCUInfo(
        name="STM32F103RC",
        family=MCUFamily.F1,
        device_id="0x20036420",
        flash_size_kb=256,
        ram_size_kb=48,
        core="Cortex-M3",
        target_config="stm32f1x.cfg",
        description="STM32F103RCT6 - High-density",
        max_clock_mhz=72
    ),
    "0x20036430": MCUInfo(
        name="STM32F103RE",
        family=MCUFamily.F1,
        device_id="0x20036430",
        flash_size_kb=512,
        ram_size_kb=64,
        core="Cortex-M3",
        target_config="stm32f1x.cfg",
        description="STM32F103RET6 - High-density",
        max_clock_mhz=72
    ),
    "0x20036440": MCUInfo(
        name="STM32F103ZC",
        family=MCUFamily.F1,
        device_id="0x20036440",
        flash_size_kb=256,
        ram_size_kb=48,
        core="Cortex-M3",
        target_config="stm32f1x.cfg",
        description="STM32F103ZCT6 - High-density",
        max_clock_mhz=72
    ),
    "0x20036450": MCUInfo(
        name="STM32F103ZE",
        family=MCUFamily.F1,
        device_id="0x20036450",
        flash_size_kb=512,
        ram_size_kb=64,
        core="Cortex-M3",
        target_config="stm32f1x.cfg",
        description="STM32F103ZET6 - High-density",
        max_clock_mhz=72
    ),
    
    # ========== STM32F4系列 ==========
    "0x10076413": MCUInfo(
        name="STM32F401CC",
        family=MCUFamily.F4,
        device_id="0x10076413",
        flash_size_kb=256,
        ram_size_kb=64,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F401CCU6 - Entry-level",
        max_clock_mhz=84
    ),
    "0x10076415": MCUInfo(
        name="STM32F401CE",
        family=MCUFamily.F4,
        device_id="0x10076415",
        flash_size_kb=512,
        ram_size_kb=96,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F401CEU6 - Entry-level",
        max_clock_mhz=84
    ),
    "0x10076419": MCUInfo(
        name="STM32F401RC",
        family=MCUFamily.F4,
        device_id="0x10076419",
        flash_size_kb=256,
        ram_size_kb=64,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F401RCT6 - Entry-level",
        max_clock_mhz=84
    ),
    "0x1007641B": MCUInfo(
        name="STM32F401RE",
        family=MCUFamily.F4,
        device_id="0x1007641B",
        flash_size_kb=512,
        ram_size_kb=96,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F401RET6 - Entry-level",
        max_clock_mhz=84
    ),
    "0x10076423": MCUInfo(
        name="STM32F401VC",
        family=MCUFamily.F4,
        device_id="0x10076423",
        flash_size_kb=256,
        ram_size_kb=64,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F401VCT6 - Entry-level",
        max_clock_mhz=84
    ),
    "0x10076425": MCUInfo(
        name="STM32F401VE",
        family=MCUFamily.F4,
        device_id="0x10076425",
        flash_size_kb=512,
        ram_size_kb=96,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F401VET6 - Entry-level",
        max_clock_mhz=84
    ),
    "0x10016419": MCUInfo(
        name="STM32F405RG",
        family=MCUFamily.F4,
        device_id="0x10016419",
        flash_size_kb=1024,
        ram_size_kb=192,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F405RGT6 - Foundation",
        max_clock_mhz=168
    ),
    "0x1001641F": MCUInfo(
        name="STM32F407VG",
        family=MCUFamily.F4,
        device_id="0x1001641F",
        flash_size_kb=1024,
        ram_size_kb=192,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F407VGT6 - Foundation",
        max_clock_mhz=168
    ),
    "0x10016433": MCUInfo(
        name="STM32F407IG",
        family=MCUFamily.F4,
        device_id="0x10016433",
        flash_size_kb=1024,
        ram_size_kb=192,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F407IGT6 - Foundation",
        max_clock_mhz=168
    ),
    "0x10016453": MCUInfo(
        name="STM32F407ZG",
        family=MCUFamily.F4,
        device_id="0x10016453",
        flash_size_kb=1024,
        ram_size_kb=192,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F407ZGT6 - Foundation",
        max_clock_mhz=168
    ),
    "0x10016449": MCUInfo(
        name="STM32F411CC",
        family=MCUFamily.F4,
        device_id="0x10016449",
        flash_size_kb=256,
        ram_size_kb=128,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F411CCU6 - Access",
        max_clock_mhz=100
    ),
    "0x1001644B": MCUInfo(
        name="STM32F411CE",
        family=MCUFamily.F4,
        device_id="0x1001644B",
        flash_size_kb=512,
        ram_size_kb=128,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F411CEU6 - Access",
        max_clock_mhz=100
    ),
    "0x10016461": MCUInfo(
        name="STM32F412CG",
        family=MCUFamily.F4,
        device_id="0x10016461",
        flash_size_kb=1024,
        ram_size_kb=256,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F412CGU6 - Dynamic Efficiency",
        max_clock_mhz=100
    ),
    "0x10016463": MCUInfo(
        name="STM32F412RG",
        family=MCUFamily.F4,
        device_id="0x10016463",
        flash_size_kb=1024,
        ram_size_kb=256,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F412RGT6 - Dynamic Efficiency",
        max_clock_mhz=100
    ),
    "0x10016469": MCUInfo(
        name="STM32F413CG",
        family=MCUFamily.F4,
        device_id="0x10016469",
        flash_size_kb=1024,
        ram_size_kb=320,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F413CGU6 - Advanced",
        max_clock_mhz=100
    ),
    "0x1001646B": MCUInfo(
        name="STM32F413RG",
        family=MCUFamily.F4,
        device_id="0x1001646B",
        flash_size_kb=1024,
        ram_size_kb=320,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F413RGT6 - Advanced",
        max_clock_mhz=100
    ),
    "0x10016479": MCUInfo(
        name="STM32F429ZI",
        family=MCUFamily.F4,
        device_id="0x10016479",
        flash_size_kb=2048,
        ram_size_kb=256,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F429ZIT6 - Advanced",
        max_clock_mhz=180
    ),
    "0x10016481": MCUInfo(
        name="STM32F446RC",
        family=MCUFamily.F4,
        device_id="0x10016481",
        flash_size_kb=256,
        ram_size_kb=128,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F446RCT6 - Advanced",
        max_clock_mhz=180
    ),
    "0x10016483": MCUInfo(
        name="STM32F446RE",
        family=MCUFamily.F4,
        device_id="0x10016483",
        flash_size_kb=512,
        ram_size_kb=128,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F446RET6 - Advanced",
        max_clock_mhz=180
    ),
    "0x1001648B": MCUInfo(
        name="STM32F446ZE",
        family=MCUFamily.F4,
        device_id="0x1001648B",
        flash_size_kb=512,
        ram_size_kb=128,
        core="Cortex-M4",
        target_config="stm32f4x.cfg",
        description="STM32F446ZET6 - Advanced",
        max_clock_mhz=180
    ),
    
    # ========== STM32F7系列 ==========
    "0x10016452": MCUInfo(
        name="STM32F745VG",
        family=MCUFamily.F7,
        device_id="0x10016452",
        flash_size_kb=1024,
        ram_size_kb=320,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F745VGT6 - Foundation",
        max_clock_mhz=216
    ),
    "0x10016453": MCUInfo(
        name="STM32F745ZG",
        family=MCUFamily.F7,
        device_id="0x10016453",
        flash_size_kb=1024,
        ram_size_kb=320,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F745ZGT6 - Foundation",
        max_clock_mhz=216
    ),
    "0x10016454": MCUInfo(
        name="STM32F750N8",
        family=MCUFamily.F7,
        device_id="0x10016454",
        flash_size_kb=64,
        ram_size_kb=320,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F750N8H6 - Advanced",
        max_clock_mhz=216
    ),
    "0x10016458": MCUInfo(
        name="STM32F756VG",
        family=MCUFamily.F7,
        device_id="0x10016458",
        flash_size_kb=1024,
        ram_size_kb=320,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F756VGT6 - Advanced",
        max_clock_mhz=216
    ),
    "0x10016463": MCUInfo(
        name="STM32F765VG",
        family=MCUFamily.F7,
        device_id="0x10016463",
        flash_size_kb=1024,
        ram_size_kb=512,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F765VGT6 - Advanced",
        max_clock_mhz=216
    ),
    "0x1001646B": MCUInfo(
        name="STM32F767ZI",
        family=MCUFamily.F7,
        device_id="0x1001646B",
        flash_size_kb=2048,
        ram_size_kb=512,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F767ZIT6 - Advanced",
        max_clock_mhz=216
    ),
    "0x10016485": MCUInfo(
        name="STM32F722RC",
        family=MCUFamily.F7,
        device_id="0x10016485",
        flash_size_kb=256,
        ram_size_kb=256,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F722RCT6 - Entry-level",
        max_clock_mhz=216
    ),
    "0x1001648B": MCUInfo(
        name="STM32F722ZC",
        family=MCUFamily.F7,
        device_id="0x1001648B",
        flash_size_kb=256,
        ram_size_kb=256,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F722ZCT6 - Entry-level",
        max_clock_mhz=216
    ),
    "0x10016495": MCUInfo(
        name="STM32F723VC",
        family=MCUFamily.F7,
        device_id="0x10016495",
        flash_size_kb=256,
        ram_size_kb=256,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F723VCT6 - Entry-level",
        max_clock_mhz=216
    ),
    "0x100164A3": MCUInfo(
        name="STM32F730R8",
        family=MCUFamily.F7,
        device_id="0x100164A3",
        flash_size_kb=64,
        ram_size_kb=256,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F730R8T6 - Value",
        max_clock_mhz=216
    ),
    "0x100164B3": MCUInfo(
        name="STM32F732RE",
        family=MCUFamily.F7,
        device_id="0x100164B3",
        flash_size_kb=512,
        ram_size_kb=256,
        core="Cortex-M7",
        target_config="stm32f7x.cfg",
        description="STM32F732RET6 - Entry-level",
        max_clock_mhz=216
    ),
    
    # ========== STM32H7系列 ==========
    "0x10006450": MCUInfo(
        name="STM32H743ZI",
        family=MCUFamily.H7,
        device_id="0x10006450",
        flash_size_kb=2048,
        ram_size_kb=1024,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H743ZIT6 - High Performance",
        max_clock_mhz=480
    ),
    "0x10006453": MCUInfo(
        name="STM32H747ZI",
        family=MCUFamily.H7,
        device_id="0x10006453",
        flash_size_kb=2048,
        ram_size_kb=1024,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H747ZIT6 - High Performance (Dual Core)",
        max_clock_mhz=480
    ),
    "0x10006460": MCUInfo(
        name="STM32H723ZG",
        family=MCUFamily.H7,
        device_id="0x10006460",
        flash_size_kb=1024,
        ram_size_kb=564,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H723ZGT6 - High Performance",
        max_clock_mhz=550
    ),
    "0x10006463": MCUInfo(
        name="STM32H725IG",
        family=MCUFamily.H7,
        device_id="0x10006463",
        flash_size_kb=1024,
        ram_size_kb=564,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H725IGT6 - High Performance",
        max_clock_mhz=550
    ),
    "0x10006470": MCUInfo(
        name="STM32H730IB",
        family=MCUFamily.H7,
        device_id="0x10006470",
        flash_size_kb=2048,
        ram_size_kb=564,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H730IBT6 - High Performance",
        max_clock_mhz=550
    ),
    "0x10006480": MCUInfo(
        name="STM32H7A3ZI",
        family=MCUFamily.H7,
        device_id="0x10006480",
        flash_size_kb=2048,
        ram_size_kb=1024,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H7A3ZIT6 - High Performance",
        max_clock_mhz=280
    ),
    "0x10006490": MCUInfo(
        name="STM32H7B3ZI",
        family=MCUFamily.H7,
        device_id="0x10006490",
        flash_size_kb=2048,
        ram_size_kb=1024,
        core="Cortex-M7",
        target_config="stm32h7x.cfg",
        description="STM32H7B3ZIT6 - High Performance",
        max_clock_mhz=280
    ),
}


# 系列到目标配置的映射
FAMILY_TARGET_MAP: Dict[MCUFamily, str] = {
    MCUFamily.F0: "stm32f0x.cfg",
    MCUFamily.F1: "stm32f1x.cfg",
    MCUFamily.F2: "stm32f2x.cfg",
    MCUFamily.F3: "stm32f3x.cfg",
    MCUFamily.F4: "stm32f4x.cfg",
    MCUFamily.F7: "stm32f7x.cfg",
    MCUFamily.H7: "stm32h7x.cfg",
    MCUFamily.L0: "stm32l0.cfg",
    MCUFamily.L1: "stm32l1.cfg",
    MCUFamily.L4: "stm32l4x.cfg",
    MCUFamily.L5: "stm32l5x.cfg",
    MCUFamily.G0: "stm32g0x.cfg",
    MCUFamily.G4: "stm32g4x.cfg",
    MCUFamily.U5: "stm32u5x.cfg",
    MCUFamily.WB: "stm32wbx.cfg",
    MCUFamily.WL: "stm32wlx.cfg",
}


def get_mcu_info(device_id: str) -> Optional[MCUInfo]:
    """根据设备ID获取MCU信息
    
    Args:
        device_id: 设备IDCODE (例如 "0x20036410")
        
    Returns:
        MCUInfo对象或None
    """
    # 标准化ID格式
    normalized_id = device_id.lower().strip()
    if not normalized_id.startswith("0x"):
        normalized_id = "0x" + normalized_id
    
    return STM32_MCU_DATABASE.get(normalized_id)


def get_target_config_by_family(family: MCUFamily) -> str:
    """根据MCU系列获取OpenOCD目标配置
    
    Args:
        family: MCU系列枚举
        
    Returns:
        OpenOCD目标配置文件名
    """
    return FAMILY_TARGET_MAP.get(family, "stm32f1x.cfg")


def detect_family_from_id(device_id: str) -> Optional[MCUFamily]:
    """从设备ID推断MCU系列
    
    Args:
        device_id: 设备IDCODE
        
    Returns:
        MCU系列枚举或None
    """
    mcu_info = get_mcu_info(device_id)
    if mcu_info:
        return mcu_info.family
    
    # 如果数据库中没有，尝试从ID推断
    # F1: 0x20036xxx
    # F4: 0x100164xx, 0x100764xx
    # F7: 0x100164xx (与F4重叠，需要更精确匹配)
    # H7: 0x100064xx
    
    device_id_lower = device_id.lower()
    
    if "0x2003" in device_id_lower:
        return MCUFamily.F1
    elif "0x100064" in device_id_lower:
        return MCUFamily.H7
    elif "0x1001" in device_id_lower:
        # F4或F7，需要更多信息
        return None
    
    return None


def list_supported_mcus() -> List[Dict[str, Any]]:
    """列出所有支持的MCU
    
    Returns:
        MCU信息列表
    """
    return [
        {
            "name": mcu.name,
            "family": mcu.family.value,
            "device_id": mcu.device_id,
            "flash_kb": mcu.flash_size_kb,
            "ram_kb": mcu.ram_size_kb,
            "core": mcu.core,
            "description": mcu.description,
        }
        for mcu in sorted(STM32_MCU_DATABASE.values(), key=lambda x: (x.family.value, x.name))
    ]


def get_supported_families() -> List[Dict[str, Any]]:
    """获取支持的MCU系列列表
    
    Returns:
        系列信息列表
    """
    families = {}
    for mcu in STM32_MCU_DATABASE.values():
        if mcu.family not in families:
            families[mcu.family] = {
                "name": mcu.family.value,
                "target_config": FAMILY_TARGET_MAP.get(mcu.family),
                "mcu_count": 0,
                "cores": set(),
                "max_clocks": [],
            }
        families[mcu.family]["mcu_count"] += 1
        families[mcu.family]["cores"].add(mcu.core)
        families[mcu.family]["max_clocks"].append(mcu.max_clock_mhz)
    
    result = []
    for family, info in sorted(families.items(), key=lambda x: x[0].value):
        result.append({
            "name": info["name"],
            "target_config": info["target_config"],
            "mcu_count": info["mcu_count"],
            "cores": list(info["cores"]),
            "max_clock_range": f"{min(info['max_clocks'])}-{max(info['max_clocks'])} MHz"
        })
    
    return result
