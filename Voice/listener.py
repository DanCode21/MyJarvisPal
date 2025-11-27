# listener 
# Records audio for a short time after the wake word.

import struct
from Audio.microphone import open_stream

def listen_for_command(rhino):
    """Record up to 3 seconds of speech and return the inference."""
    rate = rhino.sample_rate
    size = rhino.frame_length

    stream = open_stream(rate, size)

    max_seconds = 3.0
    max_frames = int(rate / size * max_seconds)
    frame_count = 0

    while True:
        data = stream.read(size, exception_on_overflow=False)
        pcm = struct.unpack_from("h" * size, data)

        is_final = rhino.process(pcm)
        frame_count += 1

        if is_final or frame_count >= max_frames:
            inference = rhino.get_inference()
            stream.close()
            return inference
