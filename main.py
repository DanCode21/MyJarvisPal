# main.py
# This file connects the wake word engine (Porcupine) and the intent engine(Rhino) + text to speech (flite)
#handles voice commands for weather, time, music playback, volume control, and sleep mode.

# ----------------------- ---------- Import files/libaries  ---------------------------------
import struct
import time
import os
from System.tts import speak, ignore_audio
from Audio.music import play_song, stop_music, pause_music, resume_music, is_music_playing, send_display_command
from Audio.microphone import open_stream, cleanup as mic_cleanup
from Voice.wakeword import detect_wake, porcupine, cleanup as wake_cleanup
from Voice.rhino import rhino, cleanup as rhino_cleanup
from Voice.listener import listen_for_command
from info_features.weather import get_weather
from info_features.system_time import get_time

# Cities that Rhino understands that we chose to train it on
CITIES = [
    "burnaby", "vancouver", "surrey", "richmond", "new york city",
    "rome", "london", "susa", "denver", "barcelona",
    "beijing", "osaka", "moscow"]

# ----------------------- Extract city from Rhino slots -----------------------
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
speak("Jarvis is online and ready for your command Sir.")
send_display_command("IDLE")

stream = open_stream(porcupine.sample_rate, porcupine.frame_length)
 # --------------------------------- Main Loop ---------------------------------

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

 # ----------------------- ---------- WEATHER ---------------------------------
                if intent == "Weather":
                    city = find_city(slots)
                    if city is None:
                        speak("I couldn't identify the city. Please try again.")
                    else:
                        report = get_weather(city)
                        print(f"DEBUG - Weather: {report}")
                        speak(report)
                    send_display_command("IDLE")

#----------------------- ---------- SONG ----------------------- ----------
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

 # ----------------------- ---------- TIME ----------------------- ----------
                elif intent == "Time":
                    city = find_city(slots)

                    if city is None:
 # ----------------------- ----------Local time----------------------- ----------
                        current_time = get_time()
                        speak(f"The time is {current_time}")
                    else:
                        current_time = get_time(city)
                        speak(f"The time in {city} is {current_time}")
                    send_display_command("IDLE")

#----------------------- ---------- STOP MUSIC -------------------------------------
                elif intent == "StopMusic":
                    stop_music()
                    speak("Music stopped.")
                    was_playing = False
                    send_display_command("IDLE")

# ----------------------- ---------- PAUSE MUSIC ---------------------------------
                elif intent == "PauseMusic":
                    if pause_music():
                        speak("Music paused.")
                        was_playing = True
                    else:
                        speak("No music is playing.")
                        send_display_command("IDLE")

# ----------------------- ---------- RESUME MUSIC ---------------------------------
                elif intent == "ResumeMusic":
                    if resume_music():
                        speak("Resuming music.")
                        was_playing = False
                    else:
                        speak("No music to resume.")
                        send_display_command("IDLE")

# ----------------------- ---------- Volume up  ---------------------------------
                elif intent == "VolumeUp":
                    os.system("amixer set Master 10%+")
                    speak("Volume increased.")
                    if not is_music_playing():
                        send_display_command("IDLE")

# ----------------------- ---------- Volume Down  ---------------------------------
                elif intent == "VolumeDown":
                    os.system("amixer set Master 10%-")
                    speak("Volume decreased.")
                    if not is_music_playing():
                        send_display_command("IDLE")

# ----------------------- ---------- Sleep  ---------------------------------
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
            if was_playing and intent not in ["Song", "StopMusic", "PauseMusic", "ResumeMusic"]:
                print("DEBUG - Auto-resuming music after command")
                resume_music()

except KeyboardInterrupt:
    print("\nStopping assistant")
# ----------------------- ---------- Clean up  ---------------------------------
finally:
    stream.close()
    mic_cleanup()
    wake_cleanup()
    rhino_cleanup()
    send_display_command("IDLE")
    print("GoodNight Sir")