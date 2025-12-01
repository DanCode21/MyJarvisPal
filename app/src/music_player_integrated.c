// music_player_integrated.c - Receives commands from voice assistant
// Listens on named pipe for commands like PLAY, PAUSE, STOP

#include "st7789_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#define PIPE_PATH "/tmp/jarvis_display"
#define MAX_COMMAND_LEN 256

// Display state
typedef enum {
    STATE_IDLE,
    STATE_LISTENING,
    STATE_PROCESSING,
    STATE_PLAYING,
    STATE_PAUSED
} DisplayState;

typedef struct {
    char song[64];
    char artist[64];
    int duration;
    int current_time;
    DisplayState state;
    int running;
} AppState;

static AppState app_state = {
    .song = "Ready",
    .artist = "Jarvis",
    .duration = 0,
    .current_time = 0,
    .state = STATE_IDLE,
    .running = 1
};

static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

// Colors
uint16_t bg_color, text_color, accent_color;

void set_colors_for_state(DisplayState state) {
    switch(state) {
        case STATE_IDLE:
            bg_color = st7789_rgb(0, 0, 40);
            text_color = st7789_rgb(255, 255, 255);
            accent_color = st7789_rgb(30, 215, 96);
            break;
        case STATE_LISTENING:
            bg_color = st7789_rgb(40, 0, 40);
            text_color = st7789_rgb(255, 255, 255);
            accent_color = st7789_rgb(255, 100, 255);
            break;
        case STATE_PROCESSING:
            bg_color = st7789_rgb(40, 30, 0);
            text_color = st7789_rgb(255, 255, 255);
            accent_color = st7789_rgb(255, 200, 0);
            break;
        case STATE_PLAYING:
        case STATE_PAUSED:
            bg_color = st7789_rgb(0, 0, 40);
            text_color = st7789_rgb(255, 255, 255);
            accent_color = st7789_rgb(30, 215, 96);
            break;
    }
}

void format_time(int seconds, char *buf) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    sprintf(buf, "%d:%02d", mins, secs);
}

void draw_idle_screen() {
    st7789_fill_screen(bg_color);
    st7789_draw_text(80, 80, "Jarvis Assistant", text_color, bg_color, 2);
    st7789_draw_text(110, 120, "Ready", accent_color, bg_color, 2);
}

void draw_listening_screen() {
    st7789_fill_screen(bg_color);
    st7789_draw_text(80, 80, "Listening...", text_color, bg_color, 3);
}

void draw_processing_screen() {
    st7789_fill_screen(bg_color);
    st7789_draw_text(70, 80, "Processing...", text_color, bg_color, 3);
}

void draw_playing_screen(const char *song, const char *artist, int current, int duration) {
    static int last_update = -1;
    
    if (current == last_update) return;
    last_update = current;
    
    // Only redraw progress area
    float progress = (duration > 0) ? (float)current / duration : 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    // Progress bar
    st7789_draw_progress_bar(20, 150, 280, 25, progress, accent_color, bg_color);
    
    // Clear and redraw time
    st7789_fill_rect(0, 185, 320, 20, bg_color);
    
    char current_time[16], total_time[16];
    format_time(current, current_time);
    format_time(duration, total_time);
    
    st7789_draw_text(20, 185, current_time, text_color, bg_color, 2);
    st7789_draw_text(240, 185, total_time, text_color, bg_color, 2);
}

void draw_full_ui() {
    pthread_mutex_lock(&state_mutex);
    DisplayState state = app_state.state;
    
    set_colors_for_state(state);
    
    switch(state) {
        case STATE_IDLE:
            draw_idle_screen();
            break;
            
        case STATE_LISTENING:
            draw_listening_screen();
            break;
            
        case STATE_PROCESSING:
            draw_processing_screen();
            break;
            
        case STATE_PLAYING:
        case STATE_PAUSED:
            st7789_fill_screen(bg_color);
            st7789_draw_text(80, 30, "Now Playing", text_color, bg_color, 2);
            st7789_draw_text(40, 70, app_state.artist, accent_color, bg_color, 2);
            st7789_draw_text(80, 100, app_state.song, text_color, bg_color, 3);
            draw_playing_screen(app_state.song, app_state.artist, 
                              app_state.current_time, app_state.duration);
            break;
    }
    
    pthread_mutex_unlock(&state_mutex);
}

void process_command(const char *cmd) {
    printf("Received command: %s\n", cmd);
    
    pthread_mutex_lock(&state_mutex);
    
    if (strncmp(cmd, "PLAY:", 5) == 0) {
        // Format: PLAY:song:artist:duration
        char *tok = strtok((char*)cmd + 5, ":");
        if (tok) strncpy(app_state.song, tok, sizeof(app_state.song) - 1);
        
        tok = strtok(NULL, ":");
        if (tok) strncpy(app_state.artist, tok, sizeof(app_state.artist) - 1);
        
        tok = strtok(NULL, ":");
        if (tok) app_state.duration = atoi(tok);
        
        app_state.current_time = 0;
        app_state.state = STATE_PLAYING;
        
    } else if (strcmp(cmd, "PAUSE") == 0) {
        app_state.state = STATE_PAUSED;
        
    } else if (strcmp(cmd, "RESUME") == 0) {
        app_state.state = STATE_PLAYING;
        
    } else if (strcmp(cmd, "STOP") == 0) {
        app_state.state = STATE_IDLE;
        app_state.current_time = 0;
        
    } else if (strcmp(cmd, "LISTENING") == 0) {
        app_state.state = STATE_LISTENING;
        
    } else if (strcmp(cmd, "PROCESSING") == 0) {
        app_state.state = STATE_PROCESSING;
        
    } else if (strcmp(cmd, "IDLE") == 0) {
        app_state.state = STATE_IDLE;
    }
    
    pthread_mutex_unlock(&state_mutex);
    
    // Redraw full UI on state change
    draw_full_ui();
}

void* command_listener_thread(void* arg) {
    int fd;
    char buffer[MAX_COMMAND_LEN];
    
    // Create named pipe if it doesn't exist
    mkfifo(PIPE_PATH, 0666);
    
    printf("Listening for commands on %s\n", PIPE_PATH);
    
    while (app_state.running) {
        // Open pipe (blocks until writer connects)
        fd = open(PIPE_PATH, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open pipe");
            sleep(1);
            continue;
        }
        
        // Read commands
        while (app_state.running) {
            ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
            if (n <= 0) break;
            
            buffer[n] = '\0';
            
            // Process each line
            char *line = strtok(buffer, "\n");
            while (line) {
                process_command(line);
                line = strtok(NULL, "\n");
            }
        }
        
        close(fd);
    }
    
    return NULL;
}

void* timer_thread(void* arg) {
    while (app_state.running) {
        usleep(100000); // 100ms
        
        pthread_mutex_lock(&state_mutex);
        
        if (app_state.state == STATE_PLAYING) {
            static int counter = 0;
            counter++;
            
            if (counter >= 10) { // Update every second
                counter = 0;
                app_state.current_time++;
                
                if (app_state.current_time >= app_state.duration && app_state.duration > 0) {
                    app_state.state = STATE_IDLE;
                    app_state.current_time = 0;
                    pthread_mutex_unlock(&state_mutex);
                    draw_full_ui();
                    continue;
                }
                
                pthread_mutex_unlock(&state_mutex);
                draw_playing_screen(app_state.song, app_state.artist,
                                  app_state.current_time, app_state.duration);
                continue;
            }
        }
        
        pthread_mutex_unlock(&state_mutex);
    }
    
    return NULL;
}

int main() {
    printf("=== Jarvis Display Controller ===\n");
    
    // Initialize display
    if (st7789_hal_init() < 0) {
        fprintf(stderr, "Display init failed\n");
        return 1;
    }
    
    // Show idle screen
    draw_full_ui();
    
    // Start threads
    pthread_t cmd_thread, timer_thread_id;
    pthread_create(&cmd_thread, NULL, command_listener_thread, NULL);
    pthread_create(&timer_thread_id, NULL, timer_thread, NULL);
    
    printf("Display controller running. Press Ctrl+C to exit.\n");
    
    // Main loop - keep program running
    while (app_state.running) {
        sleep(1);
    }
    
    // Cleanup
    app_state.running = 0;
    pthread_join(cmd_thread, NULL);
    pthread_join(timer_thread_id, NULL);
    st7789_hal_cleanup();
    unlink(PIPE_PATH);
    
    return 0;
}