#ifndef FILTER_H
#define FILTER_H
#include <stdint.h>

typedef struct {
	// Previous input samples
	double x1;
	double x2;
	// Previous output samples
	double y1;
	double y2;
	// Feed forward coefficients
	double b0;
	double b1;
	double b2;
	// Feed back coefficients
	double a0;
	double a1;
	double a2;
} BiquadVariables;

void biquad_setup_bandpass(double target_freq, double quality, BiquadVariables *p);
int16_t biquad_filter(int16_t input, BiquadVariables *p);

#endif
