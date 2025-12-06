# System/joystick_volume.py


import time
import subprocess
import threading
import os

try:
    import spidev
    SPI_AVAILABLE = True
    print("spidev library found")
except ImportError:
    print("Warning: spidev not available")
    SPI_AVAILABLE = False
    spidev = None

ADC_CHANNEL_X = 0
VOLUME_STEP = 5
MIN_VOLUME = 0
MAX_VOLUME = 100


class JoystickVolumeController:
    def __init__(self):
        self.spi = None
        self.running = False
        self.thread = None
        self.enabled = False
        self.music_check_callback = None

        # Use 'Master' for your USB audio card
        self.control_name = "Master"

        # Joystick calibration
        self.center_x = 2048

        # Volume state
        self.current_volume = None
        self.last_volume_change_time = 0
        self.volume_cooldown = 0.3  # seconds between volume changes
        self.last_sync_time = 0

        # Joystick sensitivity
        self.x_threshold = 800  # delta from center needed to trigger change

        # Cached volume reading
        self.volume_cache = None
        self.cache_time = 0
        self.cache_duration = 5  # seconds

        print("Joystick Volume Controller initialized")

    # ------------------ Music state integration ------------------

    def set_music_check_callback(self, callback):
        self.music_check_callback = callback
        print("Music check callback set")

    def enable(self):
        if not self.enabled:
            self.enabled = True
            print("Joystick volume control ENABLED")

    def disable(self):
        if self.enabled:
            self.enabled = False
            print("Joystick volume control DISABLED")

    # ------------------ Volume helpers ------------------

    def _safe_get_current_volume(self):
        
        now = time.time()

        # Use cache if fresh
        if (
            self.volume_cache is not None
            and (now - self.cache_time) < self.cache_duration
        ):
            return self.volume_cache

        try:
            result = subprocess.run(
                ["amixer", "get", self.control_name],
                capture_output=True,
                text=True,
                timeout=1,
            )
            if result.returncode == 0:
                import re

                m = re.search(r"\[(\d+)%\]", result.stdout)
                if m:
                    vol = int(m.group(1))
                    self.volume_cache = vol
                    self.cache_time = now
                    return vol
        except Exception as e:
            print(f"Error reading volume: {e}")

        # Fallback
        if self.volume_cache is not None:
            return self.volume_cache
        if self.current_volume is not None:
            return self.current_volume
        return 50

    def _read_actual_system_volume(self):
        
        print("Reading system volume...")
        vol = self._safe_get_current_volume()
        print(f"System volume: {vol}%")
        return vol

    def sync_volume(self):
        
        now = time.time()
        if now - self.last_sync_time < 10:
            return self.current_volume

        old_volume = self.current_volume
        new_volume = self._safe_get_current_volume()

        if old_volume != new_volume:
            print(f"Volume synced: {old_volume}% -> {new_volume}%")
            self.current_volume = new_volume

        self.last_sync_time = now
        return self.current_volume

    def get_current_volume(self):
        return self._safe_get_current_volume()

    def set_volume(self, volume_percent):
       
        volume_percent = max(MIN_VOLUME, min(MAX_VOLUME, volume_percent))

        try:
            result = subprocess.run(
                ["amixer", "set", self.control_name, f"{volume_percent}%"],
                capture_output=True,
                text=True,
                timeout=1,
            )
            if result.returncode == 0:
                print(f"Volume set to {volume_percent}%")
                self.current_volume = volume_percent
                self.volume_cache = volume_percent
                self.cache_time = time.time()
                return True
            else:
                print("amixer returned non-zero exit code when setting volume")
        except Exception as e:
            print(f"Error setting volume: {e}")

        return False

    def volume_up(self, step=VOLUME_STEP):
        if self.current_volume is None:
            self.current_volume = self._read_actual_system_volume()

        new_volume = min(MAX_VOLUME, self.current_volume + step)
        if self.set_volume(new_volume):
            print(f"Volume UP: {self.current_volume}%")
            return True
        return False

    def volume_down(self, step=VOLUME_STEP):
        if self.current_volume is None:
            self.current_volume = self._read_actual_system_volume()

        new_volume = max(MIN_VOLUME, self.current_volume - step)
        if self.set_volume(new_volume):
            print(f"Volume DOWN: {self.current_volume}%")
            return True
        return False

    # ------------------ SPI / Joystick ------------------

    def init(self):
        print("Initializing joystick...")

        if not SPI_AVAILABLE:
            print("Warning: spidev not installed")
            return True  # allow program to run without hardware

        try:
            self.spi = spidev.SpiDev()
            # Bus 0, device 1 (CE1, pin 26, GPIO 7)
            self.spi.open(0, 1)
            self.spi.max_speed_hz = 1_000_000
            self.spi.mode = 0
            print("SPI initialized on CE1 (pin 26, GPIO 7)")
            return True
        except Exception as e:
            print(f"SPI Error on CE1: {e}")
            return False

    def calibrate(self):
        print("Calibrating joystick...")
        time.sleep(0.5)

        if not SPI_AVAILABLE or self.spi is None:
            print("Using simulated joystick center")
            self.center_x = 2048
            return True

        samples = 20
        x_vals = []

        for _ in range(samples):
            x = self.read_adc(ADC_CHANNEL_X)
            if 0 <= x <= 4095:
                x_vals.append(x)
            time.sleep(0.02)

        if x_vals:
            self.center_x = sum(x_vals) // len(x_vals)
            print(f"Calibrated X center: {self.center_x}")
        else:
            self.center_x = 2048
            print("Using default X center")

        return True

    def read_adc(self, channel):
        if not SPI_AVAILABLE or self.spi is None:
            return 2048

        try:
            cmd = 0b11000000 | (channel << 3)
            resp = self.spi.xfer2([cmd, 0x00, 0x00])
            value = ((resp[0] & 0x01) << 11) | (resp[1] << 3) | (resp[2] >> 5)
            return value
        except Exception as e:
            print(f"Error reading ADC: {e}")
            return -1

    # ------------------ Main joystick handling loop ------------------

    def handle_joystick(self):
        if not self.enabled:
            return

        now = time.time()

        x = self.read_adc(ADC_CHANNEL_X)
        if x < 0 or x > 4095:
            return

        dx = x - self.center_x

        if abs(dx) > self.x_threshold:
            if now - self.last_volume_change_time > self.volume_cooldown:
                if dx > self.x_threshold:
                    if self.volume_up():
                        self.last_volume_change_time = now
                elif dx < -self.x_threshold:
                    if self.volume_down():
                        self.last_volume_change_time = now

    def update_music_state(self):
        if self.music_check_callback:
            try:
                if self.music_check_callback():
                    self.enable()
                else:
                    self.disable()
            except Exception as e:
                print(f"Error checking music state: {e}")

    def start(self):
        if not self.init():
            print("Failed to initialize joystick SPI")
            return False

        if not self.calibrate():
            print("Failed to calibrate joystick")
            return False

        self.current_volume = self._read_actual_system_volume()
        self.volume_cache = self.current_volume
        self.cache_time = time.time()
        print(f"Initial volume: {self.current_volume}%")

        self.running = True
        self.thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self.thread.start()
        print("Joystick controller started")
        return True

    def _monitor_loop(self):
        print("Joystick monitor loop started")
        check_counter = 0

        while self.running:
            if check_counter >= 10:
                self.update_music_state()
                check_counter = 0

            if check_counter == 0:
                self.sync_volume()

            self.handle_joystick()
            time.sleep(0.05)
            check_counter += 1

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join(timeout=2)

        if self.spi:
            try:
                self.spi.close()
            except Exception:
                pass

        print("Joystick controller stopped")


joystick_controller = JoystickVolumeController()
