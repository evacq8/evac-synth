#ifndef ALSA_MIDI_HELPER_H
#define ALSA_MIDI_HELPER_H
#include <alsa/asoundlib.h>
#include <stdint.h>
snd_seq_t* initialize_seq(void);
#endif
