#ifndef ALSA_MIDI_HELPER_H
#define ALSA_MIDI_HELPER_H
#include <alsa/asoundlib.h>
#include <stdint.h>

typedef struct {
	int client;
	int port;
} MidiAddress;

snd_seq_t* initialize_seq(void);
MidiAddress prompt_midi_senders(snd_seq_t *seq);
#endif
