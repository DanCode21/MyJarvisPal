# main.py - Core program that connects wake word, Rhino, weather, music, etc.

import struct
import time

from System.tts import speak, ignore_audio
from Audio.music import play_song, stop_music
from Audio.microphone import open_stream, cleanup as mic_cleanup
from Voice.wakeword import detect_wake, porcupine, cleanup as wake_cleanup
from Voice.rhino import rhino, cleanup as rhino_cleanup
from Voice.listener import listen_for_command
from info_features.weather import get_weather
from info_features.system_time import get_time

# Cities that the assistant understands clearly
CITIES = ["vancouver", "surrey", "richmond", "burnaby", "new york city"]

def find_city(slots):
    for key in slots:
        val = slots[key].lower()
        if val in CITIES:
            return val
    return "vancouver"

print("Assistant is running")
speak("Jarvis is online and ready for your command.")

stream = open_stream(porcupine.sample_rate, porcupine.frame_length)

try:
    while True:
        data = stream.read(porcupine.frame_length, exception_on_overflow=False)
        pcm = struct.unpack_from("h" * porcupine.frame_length, data)

        if ignore_audio:
            continue

        if detect_wake(pcm) >= 0:
            stop_music()
            speak("At your service sir.")

            time.sleep(0.4)
            inference = listen_for_command(rhino)

            if inference.is_understood:
                intent = inference.intent
                slots = inference.slots

                if intent == "Weather":
                    city = find_city(slots)
                    speak(get_weather(city))

                elif intent == "Song":
                    song_name = (
                        slots.get("song") or
                        slots.get("songs") or
                        slots.get("track") or
                        ""
                    )
                    play_song(song_name)

                elif intent == "Time":
                    speak("The time is " + get_time())

                else:
                    speak("I understood you but cannot do that yet.")
            else:
                speak(get_weather("vancouver"))

except KeyboardInterrupt:
    print("Stopping assistant")

finally:
    stream.close()
    mic_cleanup()
    wake_cleanup()
    rhino_cleanup()
    print("Goodbye.")
