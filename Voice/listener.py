# listener.py - UPDATED VERSION
import struct
from Audio.microphone import open_stream

def listen_for_command(rhino):
    rate = rhino.sample_rate
    size = rhino.frame_length
    stream = open_stream(rate, size)
    
    max_seconds = 5.0  # Increased from 3 to 5 seconds
    max_frames = int(rate / size * max_seconds)
    frame_count = 0
    
    print("Listening...")  # Debug feedback
    
    while True:
        data = stream.read(size, exception_on_overflow=False)
        pcm = struct.unpack_from("h" * size, data)
        is_final = rhino.process(pcm)
        frame_count += 1
        
        if is_final:
            print("Command detected!")  # Debug feedback
            inference = rhino.get_inference()
            stream.close()
            return inference
            
        if frame_count >= max_frames:
            print("Timeout reached")  # Debug feedback
            inference = rhino.get_inference()
            stream.close()
            return inference