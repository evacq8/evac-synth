#ifndef ENVELOPE_H
#define ENVELOPE_H

typedef enum {
	ENVELOPE_FREE, // Note is not producing sound and ready to be reused
	ENVELOPE_ATTACK, // Envelope Volume(0.0) -> 1.0
	ENVELOPE_DECAY, // Envelope Volume(1.0) -> Sustain Level
	ENVELOPE_SUSTAIN, // Envelope Volume(Sustain Level)
	ENVELOPE_RELEASE // Envelope(Current Volume) -> 0.0 (FREE)
} EnvelopeState;

void envelope_tick(float *env_level, EnvelopeState *env_state);

#endif
