import machine
import network
import time
import socket
import select

# --- Hardware Config ---
LED_PIN = 8
BUTTON_PIN = 10
SWDIO_PIN = 3
SWCLK_PIN = 4
NRST_PIN = 5

led = machine.Pin(LED_PIN, machine.Pin.OUT)
led.value(1)  # Off


# --- WiFi Setup ---
def setup_wifi():
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid="ESP32-Bridge-Setup", password="stm32bridge")
    print("AP Started:", ap.ifconfig())

    # Try connecting to STA if configured (skipped for demo)
    return ap


# --- SWD Bit-bang (Simplified) ---
class SWD:
    def __init__(self, clk, dio):
        self.clk = machine.Pin(clk, machine.Pin.OUT)
        self.dio = machine.Pin(dio, machine.Pin.OUT)
        self.clk.value(0)

    def write_bit(self, bit):
        self.dio.value(bit)
        time.sleep_us(1)
        self.clk.value(1)
        time.sleep_us(1)
        self.clk.value(0)

    def read_bit(self):
        self.dio = machine.Pin(SWDIO_PIN, machine.Pin.IN)
        time.sleep_us(1)
        self.clk.value(1)
        bit = self.dio.value()
        time.sleep_us(1)
        self.clk.value(0)
        self.dio = machine.Pin(SWDIO_PIN, machine.Pin.OUT)
        return bit

    def reset(self):
        # Line Reset
        self.dio.value(1)
        for _ in range(50):
            self.clk.value(1)
            self.clk.value(0)

        # JTAG -> SWD (0xE79E)
        seq = 0xE79E
        for i in range(16):
            self.write_bit((seq >> i) & 1)

        # Line Reset again
        self.dio.value(1)
        for _ in range(50):
            self.clk.value(1)
            self.clk.value(0)

        # Idle
        self.dio.value(0)
        for _ in range(8):
            self.clk.value(1)
            self.clk.value(0)


# --- Main Loop ---
def main():
    print("Starting ESP32-STM32 Bridge (MicroPython)")

    # Blink LED to indicate start
    for _ in range(5):
        led.value(0)
        time.sleep(0.1)
        led.value(1)
        time.sleep(0.1)

    ap = setup_wifi()

    # TCP Server
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("", 4444))
    s.listen(1)
    s.setblocking(False)
    print("TCP Server listening on 4444")

    swd = SWD(SWCLK_PIN, SWDIO_PIN)

    while True:
        try:
            readable, _, _ = select.select([s], [], [], 0.1)
            if s in readable:
                conn, addr = s.accept()
                print("Connected by", addr)
                conn.send(b"ESP32-Bridge v1.0 (MicroPython)\n")
                conn.close()

            # Heartbeat
            led.value(not led.value())
            time.sleep(1)

        except OSError as e:
            pass


if __name__ == "__main__":
    main()
