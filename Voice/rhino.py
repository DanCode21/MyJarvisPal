# rhino_engine 
# Converts speech into intents using Rhino.

import pvrhino
from System.config import ACCESS_KEY, RHINO_CONTEXT

rhino = pvrhino.create(access_key=ACCESS_KEY,context_path=RHINO_CONTEXT)

def process_frame(pcm):
    return rhino.process(pcm)

def get_inference():
    return rhino.get_inference()

def cleanup():
    rhino.delete()
