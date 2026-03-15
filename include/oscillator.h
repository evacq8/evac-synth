#ifndef OSCILLATOR_H
#define OSCILLATOR_H
#include <stdint.h>

#define OSCILLATOR_TABLE_SIZE 2048

typedef enum {
	OSCILLATOR_SINE,
	OSCILLATOR_SAW,
	OSCILLATOR_SQUARE,
	OSCILLATOR_TRIANGLE,
	OSCILLATOR_GLOTTAL
} OscillatorType;

void set_wave_table_oscillator(OscillatorType type, int16_t *table);
float get_wave_table_sample(float phase, int16_t *table);

#endif
