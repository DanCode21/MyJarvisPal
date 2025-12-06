// music_player_display_only.c - Music Player UI without audio
// Just displays the UI with simulated playback

#include "st7789_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Song metadata
typedef struct {
    char artist[64];
    char title[64];
    int duration_sec;
} SongInfo;

// Format seconds as "M:SS"
void format_time(int seconds, char *buf)
{
    int mins = seconds / 60;
    int secs = seconds % 60;
    sprintf(buf, "%d:%02d", mins, secs);
}

// Draw the player UI
void draw_ui(SongInfo *info, int current_sec)
{
    static int last_update = -1;
    
    // Only update once per second
    if (current_sec == last_update) return;
    last_update = current_sec;
    
    // Colors - MATCH MAIN COLORS!
    uint16_t bg = st7789_rgb(0, 0, 40);         // Background
    uint16_t text = st7789_rgb(255, 255, 255);  // Text
    uint16_t accent = st7789_rgb(30, 215, 96);  // Accent
    
    // Calculate progress
    float progress = 0.0f;
    if (info->duration_sec > 0) {
        progress = (float)current_sec / info->duration_sec;
        if (progress > 1.0f) progress = 1.0f;
    }
    
    // Redraw progress bar area
    st7789_draw_progress_bar(20, 150, 280, 25, progress, accent, bg);
    
    // Clear FULL WIDTH for time area (320 pixels wide, not 280!)
    st7789_fill_rect(0, 185, 320, 20, bg);
    
    // Draw time labels
    char current_time[16], total_time[16];
    format_time(current_sec, current_time);
    format_time(info->duration_sec, total_time);
    
    st7789_draw_text(20, 185, current_time, text, bg, 2);
    st7789_draw_text(240, 185, total_time, text, bg, 2);
    
    printf("\r%s / %s (%.0f%%)  ", current_time, total_time, progress * 100);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    printf("=== Music Player (Display Only) ===\n");
    printf("Simulating playback of: Post Malone - Wow\n\n");
    
    // Initialize display
    printf("Initializing display...\n");
    if (st7789_hal_init() < 0) {
        fprintf(stderr, "Display init failed\n");
        return 1;
    }
    
    // Song info
    SongInfo song;
    strncpy(song.artist, "Post Malone", sizeof(song.artist) - 1);
    strncpy(song.title, "Wow", sizeof(song.title) - 1);
    song.duration_sec = 149; // 2:29
    
    printf("Duration: %d:%02d\n", song.duration_sec / 60, song.duration_sec % 60);
    
    // Colors - USE SAME COLORS AS draw_ui()
    uint16_t bg = st7789_rgb(0, 0, 40);         // Dark blue background
    uint16_t text = st7789_rgb(255, 255, 255);  // White text
    uint16_t accent = st7789_rgb(30, 215, 96);  // Green accent
    
    // Draw static UI elements (landscape: 320x240)
    st7789_fill_screen(bg);
    st7789_draw_text(80, 30, "Now Playing", text, bg, 2);
    st7789_draw_text(40, 70, song.artist, accent, bg, 2);
    st7789_draw_text(80, 100, song.title, text, bg, 3);
    
    // Simulate playback
    printf("\nSimulating playback... (Ctrl+C to stop)\n");
    
    time_t start_time = time(NULL);
    int current_sec = 0;
    
    while (current_sec < song.duration_sec) {
        current_sec = (int)(time(NULL) - start_time);
        
        if (current_sec > song.duration_sec) {
            current_sec = song.duration_sec;
        }
        
        draw_ui(&song, current_sec);
        usleep(100000); 
    }
    
    printf("\n\nPlayback finished!\n");
    st7789_hal_cleanup(); 
    printf("Goodbye!\n");
    return 0;
}