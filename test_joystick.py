import spidev
import time

spi = spidev.SpiDev()
spi.open(0, 1)   # bus 0, CE1
spi.max_speed_hz = 1000000
spi.mode = 0

def read_adc(channel):
    cmd = 0b11000000 | (channel << 3)
    resp = spi.xfer2([cmd, 0x00, 0x00])
    value = ((resp[0] & 0x01) << 11) | (resp[1] << 3) | (resp[2] >> 5)
    return value

print("Reading joystick (CTRL+C to stop)")
while True:
    x = read_adc(0)
    print("X =", x)
    time.sleep(0.1)
