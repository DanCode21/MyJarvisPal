// music_player.c - Music Player with UI
// Plays MP3 and displays progress on ST7789

#include "st7789_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpg123.h>
#include <ao/ao.h>

// Song metadata
typedef struct {
    char artist[64];
    char title[64];
    int duration_sec;
} SongInfo;

// Audio state
static mpg123_handle *mh = NULL;
static ao_device *audio_dev = NULL;
static int is_playing = 0;

// Initialize audio system
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

// Open MP3 file
int audio_open(const char *filename, SongInfo *info)
{
    if (mpg123_open(mh, filename) != MPG123_OK) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, mpg123_strerror(mh));
        return -1;
    }
    
    // Get audio format
    long rate;
    int channels, encoding;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        fprintf(stderr, "Failed to get audio format\n");
        return -1;
    }
    
    // Setup audio output device
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
    
    // Get song duration
    off_t length = mpg123_length(mh);
    if (length != MPG123_ERR) {
        info->duration_sec = (int)(length / rate);
    } else {
        info->duration_sec = 149; // Fallback: 2:29 for "Wow"
    }
    
    return 0;
}

// Play one chunk of audio, returns 1 if finished
int audio_play_chunk(int *current_sec)
{
    if (!is_playing) return 0;
    
    unsigned char buffer[8192];
    size_t done;
    int err = mpg123_read(mh, buffer, sizeof(buffer), &done);
    
    if (err == MPG123_OK || err == MPG123_DONE) {
        if (done > 0) {
            ao_play(audio_dev, (char*)buffer, done);
        }
        
        // Update position
        off_t frame = mpg123_tell(mh);
        long rate;
        int channels, encoding;
        mpg123_getformat(mh, &rate, &channels, &encoding);
        *current_sec = (int)(frame / rate);
        
        if (err == MPG123_DONE) {
            return 1; // EOF
        }
    } else if (err != MPG123_OK) {
        fprintf(stderr, "Audio error: %s\n", mpg123_strerror(mh));
        return 1;
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
    
    // Colors
    uint16_t bg = st7789_rgb(20, 20, 30);       // Dark blue
    uint16_t text = st7789_rgb(255, 255, 255);  // White
    uint16_t accent = st7789_rgb(30, 215, 96);  // Green
    
    // Clear bottom area for progress
    st7789_fill_rect(0, 200, 240, 120, bg);
    
    // Calculate progress
    float progress = 0.0f;
    if (info->duration_sec > 0) {
        progress = (float)current_sec / info->duration_sec;
        if (progress > 1.0f) progress = 1.0f;
    }
    
    // Draw progress bar
    st7789_draw_progress_bar(20, 220, 200, 20, progress, accent, bg);
    
    // Draw time labels
    char current_time[16], total_time[16];
    format_time(current_sec, current_time);
    format_time(info->duration_sec, total_time);
    
    st7789_draw_text(20, 250, current_time, text, bg, 2);
    st7789_draw_text(160, 250, total_time, text, bg, 2);
    
    printf("\r%s / %s (%.0f%%)", current_time, total_time, progress * 100);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    const char *mp3_file = "Post Malone - _Wow._ (Official Music Video).mp3";
    if (argc > 1) {
        mp3_file = argv[1];
    }
    
    printf("=== Music Player ===\n");
    printf("File: %s\n\n", mp3_file);
    
    // Initialize display
    printf("Initializing display...\n");
    if (st7789_hal_init() < 0) {
        fprintf(stderr, "Display init failed\n");
        return 1;
    }
    
    // Initialize audio
    printf("Initializing audio...\n");
    if (audio_init() < 0) {
        st7789_hal_cleanup();
        return 1;
    }
    
    // Song info
    SongInfo song;
    strncpy(song.artist, "Post Malone", sizeof(song.artist) - 1);
    strncpy(song.title, "Wow", sizeof(song.title) - 1);
    song.duration_sec = 149;
    
    // Open MP3
    printf("Opening audio file...\n");
    if (audio_open(mp3_file, &song) < 0) {
        audio_cleanup();
        st7789_hal_cleanup();
        return 1;
    }
    
    printf("Duration: %d:%02d\n", song.duration_sec / 60, song.duration_sec % 60);
    
    // Colors
    uint16_t bg = st7789_rgb(20, 20, 30);
    uint16_t text = st7789_rgb(255, 255, 255);
    uint16_t accent = st7789_rgb(30, 215, 96);
    
    // Draw static UI elements
    st7789_fill_screen(bg);
    st7789_draw_text(40, 40, "Now Playing", text, bg, 2);
    st7789_draw_text(30, 100, song.artist, accent, bg, 2);
    st7789_draw_text(30, 130, song.title, text, bg, 3);
    
    // Start playback
    printf("\nPlaying... (Ctrl+C to stop)\n");
    is_playing = 1;
    int current_sec = 0;
    
    while (is_playing) {
        int done = audio_play_chunk(&current_sec);
        draw_ui(&song, current_sec);
        
        if (done) {
            printf("\n\nPlayback finished!\n");
            break;
        }
        
        usleep(10000); // 10ms sleep
    }
    
    // Cleanup
    audio_cleanup();
    st7789_hal_cleanup();
    
    printf("Goodbye!\n");
    return 0;
}