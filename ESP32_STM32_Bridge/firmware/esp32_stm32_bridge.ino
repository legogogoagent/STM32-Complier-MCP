/**
 * ESP32 STM32 SWD Bridge - Remote Flasher Firmware
 * 
 * 功能：
 * 1. 作为WiFi AP或STA连接到网络
 * 2. 通过TCP Socket接收固件数据
 * 3. 使用SWD接口将固件烧录到STM32
 * 4. 支持OpenOCD协议子集
 * 
 * 硬件连接：
 * - ESP32 GPIO18 -> STM32 SWDIO
 * - ESP32 GPIO19 -> STM32 SWCLK
 * - ESP32 GND   -> STM32 GND
 * - ESP32 3.3V  -> STM32 NRST (可选复位)
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "stm32_flash.h"

// ============== 配置 ==============
// WiFi模式: AP或STA
#define WIFI_MODE_AP    // 注释此行使用STA模式

#ifdef WIFI_MODE_AP
  const char* ssid = "ESP32-STM32-Bridge";
  const char* password = "stm32flash";  // 最少8字符
  const int port = 4444;  // OpenOCD默认端口
#else
  const char* ssid = "YOUR_WIFI_SSID";
  const char* password = "YOUR_WIFI_PASSWORD";
  const int port = 4444;
#endif

// SWD引脚定义 (可根据硬件修改)
#define SWDIO_PIN   18
#define SWCLK_PIN   19
#define NRST_PIN    21  // 可选复位引脚

// ============== SWD协议常量 ==============
#define SWD_OK     0
#define SWD_ERROR  1

// DP寄存器地址
#define DP_IDCODE  0x00
#define DP_ABORT   0x00
#define DP_CTRL    0x04
#define DP_RESEND  0x08
#define DP_RDBUFF  0x0C

// ============== 全局变量 ==============
WiFiServer server(port);
WiFiClient client;
bool clientConnected = false;

// 固件缓冲区 (最大256KB，可根据ESP32型号调整)
#define MAX_FIRMWARE_SIZE (256 * 1024)
uint8_t firmwareBuffer[MAX_FIRMWARE_SIZE];
uint32_t firmwareSize = 0;

// ============== SWD底层实现 ==============
// 位操作函数
void swdWriteBit(uint8_t bit) {
  digitalWrite(SWDIO_PIN, bit ? HIGH : LOW);
  delayMicroseconds(1);
  digitalWrite(SWCLK_PIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(SWCLK_PIN, LOW);
}

uint8_t swdReadBit() {
  pinMode(SWDIO_PIN, INPUT);
  delayMicroseconds(1);
  digitalWrite(SWCLK_PIN, HIGH);
  delayMicroseconds(1);
  uint8_t bit = digitalRead(SWDIO_PIN);
  digitalWrite(SWCLK_PIN, LOW);
  pinMode(SWDIO_PIN, OUTPUT);
  return bit;
}

// 字节操作
void swdWriteByte(uint8_t byte) {
  for (int i = 0; i < 8; i++) {
    swdWriteBit((byte >> i) & 1);
  }
}

uint8_t swdReadByte() {
  uint8_t byte = 0;
  for (int i = 0; i < 8; i++) {
    byte |= (swdReadBit() << i);
  }
  return byte;
}

// 写入32位数据 (LSB first)
void swdWriteWord(uint32_t word) {
  for (int i = 0; i < 32; i++) {
    swdWriteBit((word >> i) & 1);
  }
}

// 读取32位数据 (LSB first)
uint32_t swdReadWord() {
  uint32_t word = 0;
  for (int i = 0; i < 32; i++) {
    word |= ((uint32_t)swdReadBit() << i);
  }
  return word;
}

// SWD复位序列 (50+个时钟周期，SWDIO保持高)
void swdReset() {
  pinMode(SWDIO_PIN, OUTPUT);
  digitalWrite(SWDIO_PIN, HIGH);
  
  for (int i = 0; i < 52; i++) {
    digitalWrite(SWCLK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(SWCLK_PIN, LOW);
    delayMicroseconds(1);
  }
  
  // 发送JTAG-to-SWD切换序列 (0xE79C)
  swdWriteBit(1);
  swdWriteBit(1);
  swdWriteBit(0);
  swdWriteBit(1);
  swdWriteBit(1);
  swdWriteBit(1);
  swdWriteBit(0);
  swdWriteBit(0);
  swdWriteBit(1);
  swdWriteBit(1);
  swdWriteBit(1);
  swdWriteBit(0);
  swdWriteBit(0);
  swdWriteBit(1);
  swdWriteBit(1);
  swdWriteBit(0);
  
  // 再次复位
  for (int i = 0; i < 52; i++) {
    digitalWrite(SWCLK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(SWCLK_PIN, LOW);
    delayMicroseconds(1);
  }
  
  // 发送SWD空闲周期
  digitalWrite(SWDIO_PIN, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(SWCLK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(SWCLK_PIN, LOW);
    delayMicroseconds(1);
  }
}

// SWD请求包格式: Start(1) + APnDP(1) + RnW(1) + A[2:3](2) + Parity(1) + Stop(0) + Park(1)
uint8_t swdRequest(uint8_t apndp, uint8_t rnw, uint8_t addr) {
  uint8_t request = 0x81;  // Start=1, Park=1
  request |= (apndp & 1) << 6;
  request |= (rnw & 1) << 5;
  request |= (addr & 3) << 2;
  
  // 计算奇偶校验
  uint8_t parity = 0;
  uint8_t temp = request >> 1;
  for (int i = 0; i < 6; i++) {
    parity ^= (temp >> i) & 1;
  }
  request |= (parity & 1) << 3;
  
  return request;
}

// SWD事务
uint8_t swdTransfer(uint8_t request, uint32_t* data) {
  uint8_t ack;
  
  // 发送请求
  pinMode(SWDIO_PIN, OUTPUT);
  for (int i = 0; i < 8; i++) {
    swdWriteBit((request >> i) & 1);
  }
  
  // 转换方向
  pinMode(SWDIO_PIN, INPUT);
  delayMicroseconds(1);
  
  // 读取ACK (3 bits)
  ack = 0;
  for (int i = 0; i < 3; i++) {
    ack |= (swdReadBit() << i);
  }
  
  if (ack == 0x01) {  // OK
    if (request & 0x04) {  // 读操作
      *data = swdReadWord();
      uint8_t parity = swdReadBit();  // 读取校验位
      // 这里应该验证parity，暂时忽略
    } else {  // 写操作
      uint32_t writeData = *data;
      swdWriteWord(writeData);
      // 计算并发送校验位
      uint8_t parity = 0;
      for (int i = 0; i < 32; i++) {
        parity ^= (writeData >> i) & 1;
      }
      swdWriteBit(parity);
    }
    
    // 发送8个空闲周期
    pinMode(SWDIO_PIN, OUTPUT);
    digitalWrite(SWDIO_PIN, LOW);
    for (int i = 0; i < 8; i++) {
      digitalWrite(SWCLK_PIN, HIGH);
      delayMicroseconds(1);
      digitalWrite(SWCLK_PIN, LOW);
      delayMicroseconds(1);
    }
    
    return SWD_OK;
  }
  
  return SWD_ERROR;
}

// 读取DP IDCODE
uint32_t swdReadIdCode() {
  uint32_t idcode = 0;
  uint8_t request = swdRequest(0, 1, DP_IDCODE);  // DP, Read, IDCODE
  swdTransfer(request, &idcode);
  return idcode;
}

// ============== STM32 Flash操作 ==============
STM32FlashProgrammer flashProgrammer;
bool flashInitialized = false;

bool stm32InitFlash() {
  if (!flashInitialized) {
    if (!flashProgrammer.init()) {
      Serial.println("Failed to initialize Flash programmer");
      return false;
    }
    Serial.print("Detected MCU: ");
    Serial.println(flashProgrammer.getMCUName());
    flashInitialized = true;
  }
  return true;
}

bool stm32Halt() {
  if (!stm32InitFlash()) return false;
  return flashProgrammer.haltCore();
}

bool stm32EraseFlash() {
  if (!stm32InitFlash()) return false;
  
  Serial.println("Halting core...");
  if (!flashProgrammer.haltCore()) {
    Serial.println("Failed to halt core");
    return false;
  }
  
  Serial.println("Erasing Flash...");
  if (!flashProgrammer.eraseAll()) {
    Serial.println("Failed to erase Flash");
    return false;
  }
  
  Serial.println("Flash erased successfully");
  return true;
}

bool stm32WriteFlash(uint32_t address, uint8_t* data, uint32_t size) {
  if (!stm32InitFlash()) return false;
  
  // Write buffer to Flash
  if (!flashProgrammer.writeBuffer(address, data, size)) {
    Serial.println("Failed to write Flash");
    return false;
  }
  
  // Verify if requested
  if (!flashProgrammer.verifyBuffer(address, data, size)) {
    Serial.println("Flash verification failed");
    return false;
  }
  
  return true;
}

bool stm32Reset() {
  // Use hardware reset if available
  if (NRST_PIN >= 0) {
    digitalWrite(NRST_PIN, LOW);
    delay(100);
    digitalWrite(NRST_PIN, HIGH);
    delay(100);
    return true;
  }
  
  // Otherwise use software reset
  if (!stm32InitFlash()) return false;
  return flashProgrammer.resetSystem();
}

// ============== 协议处理 ==============
void sendResponse(const char* status, const char* message) {
  if (client && client.connected()) {
    char response[256];
    snprintf(response, sizeof(response), "%s: %s\n", status, message);
    client.print(response);
  }
}

void processCommand(String& command) {
  command.trim();
  
  Serial.print("收到命令: ");
  Serial.println(command);
  
  if (command.startsWith("reset")) {
    swdReset();
    uint32_t idcode = swdReadIdCode();
    char msg[64];
    snprintf(msg, sizeof(msg), "IDCODE=0x%08X", idcode);
    sendResponse("OK", msg);
  }
  else if (command.startsWith("idcode")) {
    uint32_t idcode = swdReadIdCode();
    char msg[64];
    snprintf(msg, sizeof(msg), "0x%08X", idcode);
    sendResponse("OK", msg);
  }
  else if (command.startsWith("upload ")) {
    // 上传固件: upload <size>
    int size = command.substring(7).toInt();
    if (size > 0 && size <= MAX_FIRMWARE_SIZE) {
      firmwareSize = 0;
      sendResponse("OK", "Ready for upload");
      
      // 接收固件数据
      uint32_t timeout = millis() + 30000;  // 30秒超时
      while (firmwareSize < (uint32_t)size && millis() < timeout) {
        while (client.available() && firmwareSize < (uint32_t)size) {
          firmwareBuffer[firmwareSize++] = client.read();
        }
        delay(1);
      }
      
      if (firmwareSize == (uint32_t)size) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Received %d bytes", firmwareSize);
        sendResponse("OK", msg);
      } else {
        sendResponse("ERROR", "Upload timeout or incomplete");
      }
    } else {
      sendResponse("ERROR", "Invalid size");
    }
  }
  else if (command.startsWith("flash")) {
    // 烧录固件
    if (firmwareSize == 0) {
      sendResponse("ERROR", "No firmware loaded");
      return;
    }
    
    sendResponse("INFO", "Halting target...");
    if (!stm32Halt()) {
      sendResponse("ERROR", "Failed to halt target");
      return;
    }
    
    sendResponse("INFO", "Erasing flash...");
    if (!stm32EraseFlash()) {
      sendResponse("ERROR", "Failed to erase flash");
      return;
    }
    
    sendResponse("INFO", "Programming flash...");
    if (stm32WriteFlash(0x08000000, firmwareBuffer, firmwareSize)) {
      sendResponse("OK", "Flash programming complete");
    } else {
      sendResponse("ERROR", "Flash programming failed");
    }
    
    stm32Reset();
  }
  else if (command.startsWith("version")) {
    sendResponse("OK", "ESP32-STM32-Bridge v1.0.0");
  }
  else if (command.startsWith("help")) {
    client.println("Commands:");
    client.println("  reset         - Reset SWD and read IDCODE");
    client.println("  idcode        - Read target IDCODE");
    client.println("  upload <size> - Upload firmware (binary)");
    client.println("  flash         - Flash uploaded firmware to STM32");
    client.println("  version       - Show version");
    client.println("  help          - Show this help");
  }
  else {
    sendResponse("ERROR", "Unknown command");
  }
}

// ============== 串口透传 ==============
// ESP32串口与STM32 UART连接，实现调试日志转发
#define STM32_UART_RX 16  // ESP32 RX <- STM32 TX
#define STM32_UART_TX 17  // ESP32 TX -> STM32 RX (可选)
#define STM32_UART_BAUD 115200

void setupSerialBridge() {
  Serial1.begin(STM32_UART_BAUD, SERIAL_8N1, STM32_UART_RX, STM32_UART_TX);
  Serial.println("Serial bridge started: ESP32<->STM32 UART");
}

void handleSerialBridge() {
  // 将STM32的串口输出转发到WiFi客户端
  while (Serial1.available()) {
    uint8_t byte = Serial1.read();
    if (client && client.connected()) {
      client.write(byte);
    }
    // 同时输出到ESP32串口(调试用)
    Serial.write(byte);
  }
}

// ============== 主程序 ==============
void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("ESP32 STM32 SWD Bridge");
  Serial.println("========================================\n");
  
  // 初始化SWD引脚
  pinMode(SWDIO_PIN, OUTPUT);
  pinMode(SWCLK_PIN, OUTPUT);
  digitalWrite(SWDIO_PIN, LOW);
  digitalWrite(SWCLK_PIN, LOW);
  
  // 初始化复位引脚
  if (NRST_PIN >= 0) {
    pinMode(NRST_PIN, OUTPUT);
    digitalWrite(NRST_PIN, HIGH);
  }
  
  // 初始化WiFi
#ifdef WIFI_MODE_AP
  Serial.println("启动WiFi AP模式...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP地址: ");
  Serial.println(IP);
#else
  Serial.print("连接到WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("STA IP地址: ");
  Serial.println(WiFi.localIP());
#endif
  
  // 启动TCP服务器
  server.begin();
  Serial.print("TCP服务器已启动，端口: ");
  Serial.println(port);
  Serial.println("等待客户端连接...\n");
  
  // 可选：启动串口桥
  setupSerialBridge();
}

void loop() {
  // 处理新的客户端连接
  if (!clientConnected) {
    client = server.available();
    if (client) {
      clientConnected = true;
      Serial.print("客户端已连接: ");
      Serial.println(client.remoteIP());
      client.println("ESP32-STM32-Bridge v1.0.0");
      client.println("Type 'help' for commands");
    }
  }
  
  // 处理客户端命令
  if (clientConnected) {
    if (client.connected()) {
      while (client.available()) {
        String command = client.readStringUntil('\n');
        processCommand(command);
      }
    } else {
      Serial.println("客户端断开连接");
      client.stop();
      clientConnected = false;
    }
  }
  
  // 处理串口桥
  handleSerialBridge();
  
  // 小延迟避免看门狗复位
  delay(1);
}
