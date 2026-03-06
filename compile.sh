#!/bin/bash
gcc synthesis.c alsa_audio_handler.c alsa_midi_handler.c -o build -lasound -lm

