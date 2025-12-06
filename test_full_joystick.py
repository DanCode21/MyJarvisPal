from System.joystick_volume import joystick_controller
import time

print("Initializing joystick...")
if not joystick_controller.start():
    print("Joystick failed to start")
    exit(1)

print("\nMove the joystick left/right to change volume.")
print("CTRL+C to exit.\n")

try:
    while True:
        time.sleep(0.2)
except KeyboardInterrupt:
    print("\nStopping...")
    joystick_controller.stop()
