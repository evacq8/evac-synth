#ifndef ALSA_AUDIO_HELPER_H
#define ALSA_AUDIO_HELPER_H
#include <alsa/asoundlib.h>
#include <stdint.h>
snd_pcm_t* initialize_pcm(void);
int write_to_alsa_buffer(snd_pcm_t *pcm, int16_t *buffer);
#endif
