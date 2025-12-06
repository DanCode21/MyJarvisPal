# music.py - Add helper function
import os
import signal
import subprocess
import time
from System.config import MUSIC_DIR
from System.tts import speak

DISPLAY_PIPE = "/tmp/jarvis_display"
current_music_process = None
is_paused = False

def send_display_command(command):
    try:
        # Check if pipe exists first
        if not os.path.exists(DISPLAY_PIPE):
            return
        
        subprocess.run(
            ['sudo', 'tee', DISPLAY_PIPE], 
            input=(command + '\n').encode(), 
            stdout=subprocess.DEVNULL, 
            stderr=subprocess.DEVNULL,
            timeout=0.5
        )
        print(f"Display: {command}")
    except:
        pass  # Silently fail if display not available

def is_music_playing():
    global current_music_process, is_paused
    if current_music_process is not None:
        # Check if process is still alive
        if current_music_process.poll() is None:
            return not is_paused
    return False

def stop_music():
    global current_music_process, is_paused
    if current_music_process is not None:
        try:
            os.kill(current_music_process.pid, signal.SIGTERM)
            current_music_process.wait(timeout=1)
        except:
            pass
        current_music_process = None
        is_paused = False
        send_display_command("STOP")

def pause_music():
    global current_music_process, is_paused
    if current_music_process is not None and not is_paused:
        try:
            if current_music_process.poll() is None:
                os.kill(current_music_process.pid, signal.SIGSTOP)
                is_paused = True
                send_display_command("PAUSE")
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
    global current_music_process, is_paused
    if current_music_process is not None and is_paused:
        try:
            if current_music_process.poll() is None:
                os.kill(current_music_process.pid, signal.SIGCONT)
                is_paused = False
                send_display_command("RESUME")
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

def get_song_duration(filepath):
    try:
        size = os.path.getsize(filepath)
        return max(1, int(size / 176000))
    except:
        return 180

def play_song(name):
    global current_music_process, is_paused
    
    stop_music()
    
    if not name:
        speak("You did not say a song name.")
        send_display_command("IDLE")
        return
    
    found_file = None
    for file in os.listdir(MUSIC_DIR):
        if file.lower().startswith(name.lower()) and file.endswith('.wav'):
            found_file = file
            break
    
    if found_file is None:
        speak("I cannot find that song.")
        print(f"DEBUG - Song '{name}' not found in {MUSIC_DIR}")
        send_display_command("IDLE")
        return
    
    full_path = os.path.join(MUSIC_DIR, found_file)
    print(f"DEBUG - Playing file: {full_path}")
    
    try:
        # Get song info for display
        song_name = os.path.splitext(found_file)[0]
        duration = get_song_duration(full_path)
        
        current_music_process = subprocess.Popen(
            ["aplay", full_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        is_paused = False
        
        # Send display command
        send_display_command(f"PLAY:{song_name}:Music:{duration}")
        
        speak(f"Playing {name}")
        print(f"DEBUG - Music process started: PID={current_music_process.pid}")
        
        time.sleep(0.2)
        
        if current_music_process.poll() is not None:
            print(f"DEBUG - Music process ended immediately! Return code: {current_music_process.returncode}")
            stderr = current_music_process.stderr.read().decode()
            print(f"DEBUG - Error output: {stderr}")
            current_music_process = None
            send_display_command("IDLE")
        
    except Exception as e:
        print(f"DEBUG - Error playing song: {e}")
        speak("Error playing the song.")
        current_music_process = None
        send_display_command("IDLE")