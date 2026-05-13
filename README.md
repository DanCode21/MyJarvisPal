# MyJarvisPal Voice Assistant with Visual Display

A fully functional voice-activated assistant for BeagleY-AI, featuring wake-word detection, natural language understanding, and a custom ST7789 LCD display interface.

## Project Overview

This project implements a voice assistant similar to Alexa or Google Home Mini, with integrated visual feedback through an ST7789 2.0" LCD display. The system supports:

-  **Wake-word activation** ("Hey Jarvis")
-  **Intent recognition** for weather, time, music playback, and system control
-  **Text-to-speech** responses using Flite
-  **Visual display** showing assistant state and now-playing information
-  **Local music playback** with voice control
-  **Offline operation** (except weather queries)

## Hardware 

- **BeagleY-AI Board** 
- **USB Microphone** 
- **ST7789 2.0" LCD Display** (240x320, SPI interface)
  - Wiring: DC → GPIO1_33, RST → GPIO2_8, SPI0.0
- **Speaker/Headphones** (for audio output)
- **Optional**: Joystick for volume control

