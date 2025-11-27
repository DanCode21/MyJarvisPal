// music_player.c - Music Player Application
// Plays MP3 and displays UI on ST7789 display

#include "st7789_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mpg123.h>
#include <ao/ao.h>

typedef struct {
    char artist[64];
    char title[64];
    int duration_sec;
} SongInfo;

static mpg123_handle *mh = NULL;
static ao_device *audio_dev = NULL;
static int is_playing = 0;

// Initialize audio
int audio_init(void)
{
    ao_initialize();
    mpg123_init();
    
    mh = mpg123_new(NULL, NULL);
    if (!mh) {
        fprintf(stderr, "Failed to create mpg123 handle\n");
        return -1;
    }
    
    return 0;
}

// Open MP3 file and get info
int audio_open(const char *filename, SongInfo *info)
{
    if (mpg123_open(mh, filename) != MPG123_OK) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }
    
    // Get format
    long rate;
    int channels, encoding;
    mpg123_getformat(mh, &rate, &channels, &encoding);
    
    // Setup audio output
    ao_sample_format format;
    format.bits = mpg123_encsize(encoding) * 8;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    
    int driver = ao_default_driver_id();
    audio_dev = ao_open_live(driver, &format, NULL);
    if (!audio_dev) {
        fprintf(stderr, "Failed to open audio device\n");
        return -1;
    }
    
    // Get song length
    off_t length = mpg123_length(mh);
    if (length != MPG123_ERR) {
        info->duration_sec = (int)(length / rate);
    } else {
        info->duration_sec = 0; // Unknown duration
    }
    
    return 0;
}

// Play audio (call this in a loop)
int audio_play_chunk(int *current_sec)
{
    if (!is_playing) return 0;
    
    unsigned char buffer[4096];
    size_t done;
    int err = mpg123_read(mh, buffer, sizeof(buffer), &done);
    
    if (err == MPG123_OK || err == MPG123_DONE) {
        if (done > 0) {
            ao_play(audio_dev, (char*)buffer, done);
        }
        
        // Update current position
        off_t frame = mpg123_tell(mh);
        long rate;
        int channels, encoding;
        mpg123_getformat(mh, &rate, &channels, &encoding);
        *current_sec = (int)(frame / rate);
        
        if (err == MPG123_DONE) {
            return 1; // EOF
        }
    }
    
    return 0;
}

void audio_cleanup(void)
{
    is_playing = 0;
    if (audio_dev) {
        ao_close(audio_dev);
        audio_dev = NULL;
    }
    if (mh) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mh = NULL;
    }
    mpg123_exit();
    ao_shutdown();
}

// Format time as MM:SS
void format_time(int seconds, char *buf)
{
    int mins = seconds / 60;
    int secs = seconds % 60;
    sprintf(buf, "%d:%02d", mins, secs);
}

// Draw the music player UI
void draw_player_ui(SongInfo *info, int current_sec)
{
    static int last_sec = -1;
    
    // Only redraw if time changed
    if (current_sec == last_sec) return;
    last_sec = current_sec;
    
    // Colors
    uint16_t bg = st7789_rgb(20, 20, 30);      // Dark blue
    uint16_t text = st7789_rgb(255, 255, 255); // White
    uint16_t accent = st7789_rgb(30, 215, 96); // Spotify green
    
    // Clear progress area
    st7789_fill_rect(0, 200, DISPLAY_WIDTH, 100, bg);
    
    // Calculate progress
    float progress = 0.0f;
    if (info->duration_sec > 0) {
        progress = (float)current_sec / info->duration_sec;
        if (progress > 1.0f) progress = 1.0f;
    }
    
    // Draw progress bar
    st7789_draw_progress_bar(20, 220, DISPLAY_WIDTH-40, 20, progress, accent, bg);
    
    // Draw time labels
    char current_time[16], total_time[16];
    format_time(current_sec, current_time);
    format_time(info->duration_sec, total_time);
    
    st7789_draw_text(20, 250, current_time, text, bg, 2);
    
    // Right-align total time
    int total_len = strlen(total_time);
    st7789_draw_text(DISPLAY_WIDTH - 20 - (total_len * 12), 250, total_time, text, bg, 2);
}

int main(int argc, char *argv[])
{
    const char *mp3_file = "Wow.mp3"; // Default file
    if (argc > 1) {
        mp3_file = argv[1];
    }
    
    printf("Music Player\n");
    printf("============\n");
    printf("Playing: %s\n\n", mp3_file);
    
    // Initialize display
    if (st7789_hal_init() < 0) {
        fprintf(stderr, "Failed to initialize display\n");
        return 1;
    }
    
    // Initialize audio
    if (audio_init() < 0) {
        st7789_hal_cleanup();
        return 1;
    }
    
    // Song info
    SongInfo song;
    strcpy(song.artist, "Post Malone");
    strcpy(song.title, "Wow");
    song.duration_sec = 149; // Will be updated when file opens
    
    // Open MP3
    if (audio_open(mp3_file, &song) < 0) {
        fprintf(stderr, "Failed to open audio file\n");
        audio_cleanup();
        st7789_hal_cleanup();
        return 1;
    }
    
    // Colors
    uint16_t bg = st7789_rgb(20, 20, 30);
    uint16_t text = st7789_rgb(255, 255, 255);
    uint16_t accent = st7789_rgb(30, 215, 96);
    
    // Draw static UI
    st7789_fill_screen(bg);
    st7789_draw_text(20, 40, "Now Playing", text, bg, 2);
    st7789_draw_text(30, 100, song.artist, accent, bg, 2);
    st7789_draw_text(30, 130, song.title, text, bg, 3);
    
    // Start playback
    is_playing = 1;
    int current_sec = 0;
    
    printf("Playing... (Press Ctrl+C to stop)\n");
    
    // Playback loop
    while (is_playing) {
        int done = audio_play_chunk(&current_sec);
        
        // Update UI
        draw_player_ui(&song, current_sec);
        
        if (done) {
            printf("\nPlayback finished!\n");
            break;
        }
        
        usleep(10000); // 10ms sleep to prevent busy loop
    }
    
    // Cleanup
    audio_cleanup();
    st7789_hal_cleanup();
    
    return 0;
}