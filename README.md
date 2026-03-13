# evac-synth

A terrible (currently work in progress) synthesizer I'm trying to make to learn DSP, written in C. It currently uses ALSA for playing audio and connecting to a MIDI input device, and Termios for live input, so it does not work on windows.

Also, currently, it is a standalone program and not a plugin, so you cannot easily use it in a DAW. (Unless you do some finicking + high latency).

This project probably won't get far but atleast I'll learn from it.

# Current Stuff

* Takes in MIDI Input + adjusts volume based on velocity
* Selection Menu for MIDI Devices
* It's Polyphonic [Wow what a huge accomplishment (sarcasm)]
* Really bad soft clipping (tanhf)
* Pre-generated oscillator wave look-up table with linear interpolation
* Sine, Saw, Triangle, Square oscillator functions

# To-do

Next:
- [x] More Oscillators: Square, Sawtooth, Triangle
- [x] Attack, Decay, Sustain, Release Envelopes
- [ ] Handle clipping better
- [ ] Biquad Filter (for High/low/band pass filters and formants)

Later:
- [ ] Delay & Reverb
- [ ] Band-Pass Filters (& Formants)
- [ ] Overtones

Technical Stuff:
- [x] MIDI Device selector menus
- [ ] Clean up MIDI setup function
- [ ] PCM Cleanup Function
