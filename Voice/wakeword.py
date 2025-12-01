# wakeword 
# Detects wake word using Porcupine.

import pvporcupine
from System.config import ACCESS_KEY, WAKEWORD_PATH

porcupine = pvporcupine.create(access_key=ACCESS_KEY,
keyword_paths=[WAKEWORD_PATH]
)

def detect_wake(pcm):
    return porcupine.process(pcm)

def cleanup():
    porcupine.delete()
