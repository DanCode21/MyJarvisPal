# main.py
# Combined display + joystick main program
# Display behavior unchanged. Joystick adjusts system Master volume only.

import struct
import time
import os
from System.tts import speak, ignore_audio
from Audio.music import (
    play_song,
    stop_music,
    pause_music,
    resume_music,
    is_music_playing,
    send_display_command,
)
from Audio.microphone import open_stream, cleanup as mic_cleanup
from Voice.wakeword import detect_wake, porcupine, cleanup as wake_cleanup
from Voice.rhino import rhino, cleanup as rhino_cleanup
from Voice.listener import listen_for_command
from info_features.weather import get_weather
from info_features.system_time import get_time

try:
    from System.joystick_volume import joystick_controller
    JOYSTICK_AVAILABLE = True
    print("Joystick controller imported")
except ImportError as e:
    print(f"Warning importing joystick controller: {e}")
    JOYSTICK_AVAILABLE = False

    class DummyJoystickController:
        def __init__(self):
            self.current_volume = None
            self.volume_cache = None
            self.cache_time = 0

        def start(self):
            print("Joystick not available (dummy). Attempting to seed volume from system...")
            try:
                import subprocess, re

                result = subprocess.run(
                    ["amixer", "-M", "sget", "Master"],
                    capture_output=True,
                    text=True,
                    timeout=2,
                )
                if result.returncode == 0:
                    match = re.search(r"\[(\d+)%\]", result.stdout)
                    if match:
                        self.current_volume = int(match.group(1))
            except Exception:
                self.current_volume = 50
            return True

        def stop(self):
            print("Dummy joystick stopped")

        def set_music_check_callback(self, callback):
            # No-op for dummy
            pass

        def sync_volume(self):
            return self.current_volume

        def get_current_volume(self):
            return self.current_volume if self.current_volume is not None else 50

    joystick_controller = DummyJoystickController()

# Cities Rhino was trained on
CITIES = [
    "burnaby",
    "vancouver",
    "surrey",
    "richmond",
    "new york city",
    "rome",
    "london",
    "susa",
    "denver",
    "barcelona",
    "beijing",
    "osaka",
    "moscow",
]


def find_city(slots):
    """Extract city from Rhino slots."""
    print(f"DEBUG - Looking for city in slots: {slots}")

    possible_keys = ["location", "locationForWeather", "locationforweather", "city"]

    # Try expected keys first
    for key in possible_keys:
        if key in slots:
            val = slots[key].lower()
            print(f"DEBUG - Found '{key}' slot: {val}")
            if val in CITIES:
                return val

    # Fallback: search all slots
    for key, val in slots.items():
        val_lower = val.lower()
        print(f"DEBUG - Checking slot '{key}': '{val_lower}'")
        if val_lower in CITIES:
            return val_lower

    return None


print("Assistant is running")

# Initialize joystick 
if JOYSTICK_AVAILABLE:
    print("Initializing joystick...")
    try:
        joystick_controller.set_music_check_callback(is_music_playing)
    except Exception as e:
        print(f"Warning setting joystick music callback: {e}")

    try:
        if not joystick_controller.start():
            print("Warning: Joystick failed to start")
        else:
            print("✓ Joystick ready")
    except Exception as e:
        print(f"Warning starting joystick: {e}")
else:
    print("Joystick disabled (using dummy)")

# Display initial state (unchanged from display version)
speak("Jarvis is online and ready for your command Sir.")
send_display_command("IDLE")

# Open mic stream using porcupine parameters
stream = open_stream(porcupine.sample_rate, porcupine.frame_length)

try:
    while True:

        data = stream.read(porcupine.frame_length, exception_on_overflow=False)
        pcm = struct.unpack_from("h" * porcupine.frame_length, data)

        if ignore_audio:
            continue

        # Wake word detected
        if detect_wake(pcm) >= 0:
            print("\n=== WAKE WORD DETECTED ===")
            send_display_command("LISTENING")

            was_playing = is_music_playing()
            if was_playing:
                pause_music()
                print("DEBUG - Music paused for wake word")

            speak("At your service sir.")
            time.sleep(0.5)

            send_display_command("PROCESSING")

            # Listen for Rhino command
            inference = listen_for_command(rhino)

            # Reset Rhino for next command
            rhino.reset()

            print(f"\n=== INFERENCE RESULT ===")
            print(f"Understood: {inference.is_understood}")
            print(f"Intent: {inference.intent if inference.is_understood else 'N/A'}")
            print(f"Slots: {inference.slots if inference.is_understood else 'N/A'}")
            print("========================\n")

            if inference.is_understood:

                intent = inference.intent
                slots = inference.slots

                # WEATHER
                if intent == "Weather":
                    city = find_city(slots)
                    if city is None:
                        speak("I couldn't identify the city. Please try again.")
                    else:
                        report = get_weather(city)
                        print(f"DEBUG - Weather: {report}")
                        speak(report)
                    send_display_command("IDLE")

                # SONG
                elif intent == "Song":
                    song_name = slots.get("songs", "")
                    print(f"DEBUG - Song name extracted: '{song_name}'")

                    if song_name:
                        play_song(song_name)
                        time.sleep(0.5)
                        was_playing = False
                    else:
                        speak("Please tell me which song to play.")
                        send_display_command("IDLE")

                # TIME
                elif intent == "Time":
                    city = find_city(slots)

                    if city is None:
                        current_time = get_time()
                        speak(f"The time is {current_time}")
                    else:
                        current_time = get_time(city)
                        speak(f"The time in {city} is {current_time}")
                    send_display_command("IDLE")

                # STOP MUSIC
                elif intent == "StopMusic":
                    stop_music()
                    speak("Music stopped.")
                    was_playing = False
                    send_display_command("IDLE")

                # PAUSE MUSIC
                elif intent == "PauseMusic":
                    if pause_music():
                        speak("Music paused.")
                        was_playing = True
                    else:
                        speak("No music is playing.")
                        send_display_command("IDLE")

                # RESUME MUSIC
                elif intent == "ResumeMusic":
                    if resume_music():
                        speak("Resuming music.")
                        was_playing = False
                    else:
                        speak("No music to resume.")
                        send_display_command("IDLE")

                # VOLUME UP 
                elif intent == "VolumeUp":
                    os.system("amixer set Master 10%+")
                    speak("Volume increased.")
                    # Keep joystick internal state in sync if present
                    try:
                        if joystick_controller is not None:
                            if getattr(joystick_controller, "current_volume", None) is None:
                                joystick_controller.current_volume = joystick_controller.get_current_volume()
                            joystick_controller.current_volume += 10
                            if joystick_controller.current_volume > 100:
                                joystick_controller.current_volume = 100
                            joystick_controller.volume_cache = joystick_controller.current_volume
                            joystick_controller.cache_time = time.time()
                            print(f"DEBUG - Updated joystick volume: {joystick_controller.current_volume}%")
                    except Exception as e:
                        print(f"Warning updating joystick volume cache: {e}")

                    if not is_music_playing():
                        send_display_command("IDLE")

                # VOLUME DOWN 
                elif intent == "VolumeDown":
                    os.system("amixer set Master 10%-")
                    speak("Volume decreased.")
                    # Keep joystick internal state in sync if present
                    try:
                        if joystick_controller is not None:
                            if getattr(joystick_controller, "current_volume", None) is None:
                                joystick_controller.current_volume = joystick_controller.get_current_volume()
                            joystick_controller.current_volume -= 10
                            if joystick_controller.current_volume < 0:
                                joystick_controller.current_volume = 0
                            joystick_controller.volume_cache = joystick_controller.current_volume
                            joystick_controller.cache_time = time.time()
                            print(f"DEBUG - Updated joystick volume: {joystick_controller.current_volume}%")
                    except Exception as e:
                        print(f"Warning updating joystick volume cache: {e}")

                    if not is_music_playing():
                        send_display_command("IDLE")

                # SLEEP
                elif intent == "Sleep":
                    speak("Goodbye sir. Going to sleep.")
                    send_display_command("IDLE")
                    break

                else:
                    speak(f"I understood the intent {intent}, but I cannot handle it yet.")
                    if not is_music_playing():
                        send_display_command("IDLE")

            else:
                speak("Sorry, I didn't understand that. Please try again.")
                send_display_command("IDLE")

            # Auto-resume music if needed
            if was_playing and inference.is_understood:
                # If the user's command was not a music control, resume
                if inference.intent not in ["Song", "StopMusic", "PauseMusic", "ResumeMusic"]:
                    print("DEBUG - Auto-resuming music after command")
                    resume_music()

except KeyboardInterrupt:
    print("\nStopping assistant")
finally:
    # Clean up resources
    try:
        stream.close()
    except Exception:
        pass

    try:
        mic_cleanup()
    except KeyboardInterrupt:
        print("\nForce stopping...")
        os._exit(0)
    except Exception:
        pass

    try:
        wake_cleanup()
    except KeyboardInterrupt:
        print("\nForce stopping...")
        os._exit(0)
    except Exception:
        pass

    try:
        rhino_cleanup()
    except KeyboardInterrupt:
        print("\nForce stopping...")
        os._exit(0)
    except Exception:
        pass

    try:
        joystick_controller.stop()
    except KeyboardInterrupt:
        print("\nForce stopping...")
        os._exit(0)
    except Exception:
        pass

    # Leave display in idle when exiting
    try:
        send_display_command("IDLE")
    except Exception:
        pass

    print("GoodNight Sir")
