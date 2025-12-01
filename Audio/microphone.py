# microphone 
# Opens audio input stream for wake word and Rhino.

import pyaudio

pa = pyaudio.PyAudio()

def open_stream(rate, frame_length):
    return pa.open(
        rate=rate,
        channels=1,
        format=pyaudio.paInt16,
        input=True,
        frames_per_buffer=frame_length)

def cleanup():
    pa.terminate()
