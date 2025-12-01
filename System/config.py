#Config.py
import os
# This finds the folder that contains your entire project
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Picovoice Access Key (used by Porcupine + Rhino)
# OpenWeather API key

ACCESS_KEY = "tkunliDUs0stMcH4IC1yc/5Xfg2IvOPcDZKEwEe/luB12+ZKHr90AQ=="
WEATHER_API_KEY = "7f708d95554c88394c6828ae70bd9ad7"

# WakeWord + Rhino model paths
WAKEWORD_PATH = os.path.join(
    BASE_DIR,"API_Models","WakeWord","hey-jarvis_en_raspberry-pi_v3_0_0.ppn")

RHINO_CONTEXT = os.path.join(
    BASE_DIR,"API_Models","Rhino",
   "Google-Home-Mini_en_raspberry-pi_v3_0_0.rhn")

# Audio folders & files
MUSIC_DIR = os.path.join(BASE_DIR,"Audio","WaveFiles")
TTS_WAV = os.path.join(BASE_DIR,"Audio","tts.wav")
