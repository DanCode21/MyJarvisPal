import numpy as np
from scipy.signal import resample_poly

def convert_audio(stereo_bytes, input_channels=2): # Convertion for stereo headphones
    data = np.frombuffer(stereo_bytes, dtype=np.int16)

    # Split into L/R
    left = data[0::2]
    right = data[1::2]

    # Downmix
    mono = ((left.astype(np.int32) + right.astype(np.int32)) // 2).astype(np.int16)

    # Resample 48000 → 16000 (divide by 3)
    mono_16k = resample_poly(mono, up=1, down=3).astype(np.int16)

    return mono_16k
