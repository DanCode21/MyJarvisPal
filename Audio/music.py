# music.py - Add helper function
import os
import signal
import subprocess
import time
from System.config import MUSIC_DIR
from System.tts import speak

current_music_process = None
is_paused = False

def is_music_playing():
    """Check if music is currently playing (not paused, not stopped)."""
    global current_music_process, is_paused
    if current_music_process is not None:
        # Check if process is still alive
        if current_music_process.poll() is None:
            return not is_paused
    return False

def stop_music():
    """Stop currently playing music."""
    global current_music_process, is_paused
    if current_music_process is not None:
        try:
            os.kill(current_music_process.pid, signal.SIGTERM)
            current_music_process.wait(timeout=1)
        except:
            pass
        current_music_process = None
        is_paused = False

def pause_music():
    """Pause currently playing music. Returns True if successful."""
    global current_music_process, is_paused
    if current_music_process is not None and not is_paused:
        try:
            if current_music_process.poll() is None:
                os.kill(current_music_process.pid, signal.SIGSTOP)
                is_paused = True
                print("DEBUG - Music paused successfully")
                return True
            else:
                print("DEBUG - Music process already ended")
                current_music_process = None
        except Exception as e:
            print(f"DEBUG - Error pausing: {e}")
    else:
        print(f"DEBUG - Cannot pause: process={current_music_process}, paused={is_paused}")
    return False

def resume_music():
    """Resume paused music. Returns True if successful."""
    global current_music_process, is_paused
    if current_music_process is not None and is_paused:
        try:
            if current_music_process.poll() is None:
                os.kill(current_music_process.pid, signal.SIGCONT)
                is_paused = False
                print("DEBUG - Music resumed successfully")
                return True
            else:
                print("DEBUG - Music process died")
                current_music_process = None
                is_paused = False
        except Exception as e:
            print(f"DEBUG - Error resuming: {e}")
            current_music_process = None
            is_paused = False
    else:
        print(f"DEBUG - Cannot resume: process={current_music_process}, paused={is_paused}")
    return False

def play_song(name):
    """Play the first song whose filename starts with the spoken name."""
    global current_music_process, is_paused
    
    stop_music()
    
    if not name:
        speak("You did not say a song name.")
        return
    
    found_file = None
    for file in os.listdir(MUSIC_DIR):
        if file.lower().startswith(name.lower()) and file.endswith('.wav'):
            found_file = file
            break
    
    if found_file is None:
        speak("I cannot find that song.")
        print(f"DEBUG - Song '{name}' not found in {MUSIC_DIR}")
        return
    
    full_path = os.path.join(MUSIC_DIR, found_file)
    print(f"DEBUG - Playing file: {full_path}")
    
    try:
        current_music_process = subprocess.Popen(
            ["aplay", full_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        is_paused = False
        speak(f"Playing {name}")
        print(f"DEBUG - Music process started: PID={current_music_process.pid}")
        
        time.sleep(0.2)
        
        if current_music_process.poll() is not None:
            print(f"DEBUG - Music process ended immediately! Return code: {current_music_process.returncode}")
            stderr = current_music_process.stderr.read().decode()
            print(f"DEBUG - Error output: {stderr}")
            current_music_process = None
        
    except Exception as e:
        print(f"DEBUG - Error playing song: {e}")
        speak("Error playing the song.")
        current_music_process = None