/* This file focuses one oscillator functions and generating pre-generating wavetables on startup so functions do not need to be run thousands of times per second */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <oscillator.h>

#define LOOK_UP_TABLE_SIZE 2048


// OSCILLATOR FUNCTIONS BELOW:

int16_t sine_oscillator(float phase) {
	// Turn 0.0-1.0 phase into radians (multiply by 2π)
	float sample = sinf(phase*2*M_PI);
	// Multiply to match uint16_t range
	sample*=32767;
	return (int16_t)sample;
}
int16_t square_oscillator(float phase) {
	return (phase < 0.5f) ? 32767 : -32767;
}
int16_t saw_oscillator(float phase) {
	return (1.0f-2.0f*phase)*32767;
}
int16_t triangle_oscillator(float phase) {
	return (int16_t)((fabsf(phase - 0.5f) * 4.0f - 1.0f) * 32767.0f);
}
// OSCILLATOR FUNCTION END


// Look up table for oscillator outputs for normalized period values from 0-1
// Stores at int16_t (instead of float) for memory saving
int16_t wave_lookup_table[LOOK_UP_TABLE_SIZE];

// Takes in a function pointer (for an oscillator function)
// Then it populates the wave_lookup_table using that function
void populate_wave_table(int16_t (*function)(float)) {
	for(int i=0; i < LOOK_UP_TABLE_SIZE; i++) {
		wave_lookup_table[i] = function((float)i/(float)LOOK_UP_TABLE_SIZE);
	}
	printf("Populated Wave Table.\n");
}

// This higher-level function populates wavetable with the OscillatorType ENUM used to represent different oscillators using the previous function
void set_wave_table_oscillator(OscillatorType type) {
	switch(type) {
		case OSCILLATOR_SINE:
			populate_wave_table(sine_oscillator);
			break;
		case OSCILLATOR_SQUARE:
			populate_wave_table(square_oscillator);
			break;
		case OSCILLATOR_SAW:
			populate_wave_table(saw_oscillator);
			break;
		case OSCILLATOR_TRIANGLE:
			populate_wave_table(triangle_oscillator);
			break;
		default:
			break;
	}
}

// This function gets a sample from the lookup table for the given phase
// Make sure you pass a normalized phase between 0.0-1.0
float get_wave_table_sample(float phase) {

	float f_idx = phase*LOOK_UP_TABLE_SIZE; // Float index
	// Linearly interpolate between the two nearest samples.
	int d_idx = (int)f_idx % LOOK_UP_TABLE_SIZE; // Integer index (floor for positive numbers, ceil for negative numbers)
	int16_t sample1 = wave_lookup_table[d_idx];
	// Use modulo by size to prevent accessing index [table_size+1] if we're on the very last sample
	int16_t sample2 = wave_lookup_table[(d_idx+1) % LOOK_UP_TABLE_SIZE];

	return (float)((f_idx-d_idx)*(sample2 - sample1) + sample1)/32768;
}

