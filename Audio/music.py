# music - Handles search and playback of .wav music files.

import os
import signal
import subprocess
from System.config import MUSIC_DIR
from System.tts import speak

current_music_process = None

def stop_music():
    """Stop currently playing music."""
    global current_music_process
    if current_music_process is not None:
        try:
            os.kill(current_music_process.pid, signal.SIGTERM)
        except:
            pass
        current_music_process = None

def play_song(name):
    """Play the first song whose filename starts with the spoken name."""
    global current_music_process

    stop_music()

    if not name:
        speak("You did not say a song name.")
        return

    for file in os.listdir(MUSIC_DIR):
        if file.lower().startswith(name.lower()):
            full_path = os.path.join(MUSIC_DIR, file)
            current_music_process = subprocess.Popen(["aplay", full_path])
            speak(f"Playing {name}")
            return

    speak("I cannot find that song.")
