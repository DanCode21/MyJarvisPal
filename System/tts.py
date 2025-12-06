# tts 
# Handles text to speech using flite.

import os
from System.config import TTS_WAV

ignore_audio = False

def speak(text):
    global ignore_audio

    print("[Assistant]:", text)

    # Don't let it hear itself
    ignore_audio = True

    os.system(f'flite -t "{text}" -o {TTS_WAV}')
    os.system(f'aplay {TTS_WAV}')

    ignore_audio = False
