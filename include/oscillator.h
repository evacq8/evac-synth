#ifndef OSCILLATOR_H
#define OSCILLATOR_H
#include <stdint.h>

typedef enum {
	OSCILLATOR_SINE,
	OSCILLATOR_SAW,
	OSCILLATOR_SQUARE,
	OSCILLATOR_TRIANGLE
} OscillatorType;

void set_wave_table_oscillator(OscillatorType type);
float get_wave_table_sample(float phase);

#endif
