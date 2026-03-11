Hi! This is just a dumb synth project I'm doing for fun to learn audio programming.
I really suck at programming and I'm sure theres way better ways of doing things, I'm just doing them the easiest way based on what I found online.

**main.c**
* Entry point

**alsa_audio_handler.c/.h**
* Handles creating PCM device and writing to buffer for ALSA linux

**alsa_midi_handler.c/.h**
* Handles creating MIDI Seq and getting input from ALSA linux

**synthesis.c/.h**
* Contains functions that generate synth noises and notes

**cli_utils.c/.h**
* Helper functions for cli menus n stuff

# To-do
- [x] Open PCM device
- [x] Configure hardware parameters
- [x] Prepare PCM device
- [x] Generate test audio samples
- [x] Write samples
- [ ] PCM Cleanup Function
- [x] Write To Buffer Function
- [x] Link files alsa_audio_handler.c with synthesis.c
- [x] Add phase accumulator to synth
- [x] Set up alsa_midi_handler.c
- [x] Play notes with midi controller
- [x] Create a struct to manage currently held-down notes (with their velocities)
- [x] Figure out how to add two tones
- [x] Make synth polyphonetic
- [x] Soft clipping using tanhf
- [ ] Attack, Decay, Sustain, Release Envelopes
- [ ] PCM & Midi Device selector menus

