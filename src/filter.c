/* This file stores filter functions which change sound of the current sample */

#include <math.h>
#include <stdint.h>
#include "filter.h"

// This function takes in a BiquadVariables pointer and sets up coefficients for a bandpass filter
void biquad_setup_bandpass(double target_freq, double quality, BiquadVariables *p) {
	double angular_freq = 2*M_PI * target_freq/44100; // radians per sample (omega)
	double sine = sin(angular_freq);
	double cosine = cos(angular_freq);
	double alpha = sine / (2.0 * quality);

	p->b0 = alpha;
	p->b1 = 0.0;
	p->b2 = -alpha;
	p->a0 = 1.0 + alpha;
	p->a1 = -2.0 * cosine;
	p->a2 = 1.0 - alpha;

	// zero history
	p->x1 = 0;
	p->x2 = 0;
	p->y1 = 0;
	p->y2 = 0;
}

// Biquad filter function. Takes in current input sample and BiquadVariable struct pointer which stores coefficient settings and prev-sample info.
// Note: all sample values are normalized between -1.0 - 1.0
int16_t biquad_filter(int16_t input, BiquadVariables *p) {
	double input_n = (double)input/32767; // Input normalized
	double output = (p->b0 * input_n) + (p->b1 * p->x1) + (p->b2 * p->x2) - (p->a1 * p->y1) - (p->a2 * p->y2);
	output /= p->a0;

	// Update history
	p->x2 = p->x1;
	p->x1 = input_n;

	p->y2 = p->y1;
	p->y1 = output;

	// Clamp output after saving so its never greater than 1 or -1 before multiplying by 32767 while denormalizing
	if(output>1) output = 1;
	if(output<-1) output = -1;
	return (int16_t)(output*32767); // Denormalize
}
