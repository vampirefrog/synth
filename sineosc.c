#include <math.h>

#include "sineosc.h"
#include "config.h"

void sine_osc_init(struct SineOsc *osc) {
	osc->s0 = 1.0;
	osc->s1 = 0.0;
}

void sine_osc_set_freq(struct SineOsc *osc, float freq) {
	osc->a = 2.0 * sinf(M_PI * freq / SAMPLE_RATE);
}

float sine_osc_sample(struct SineOsc *osc) {
	osc->s0 = osc->s0 - osc->a * osc->s1;
	osc->s1 = osc->s1 + osc->a * osc->s0;
	return osc->s0;
}
