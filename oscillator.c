#include "oscillator.h"
#include "config.h"

int16_t oscillator_render_sample(struct Oscillator *osc) {
	if(osc->period == 0.0f) return 0;

	// Advance phase by 1 sample
	osc->phase += 1.0f;
	osc->sub_phase += 1.0f;

	float sub_period = osc->period * 2.0f;

	while(osc->phase > osc->period)
		osc->phase -= osc->period;
	while(osc->sub_phase > sub_period)
		osc->sub_phase -= sub_period;

	// Example output: simple saw + sub oscillator
	return (int16_t)((1 << 13) - (osc->phase * 8.0f) / (osc->period / 2048.0f) + ((osc->sub_phase - osc->period / 2.0f > osc->period) ? 4096 : -4096));
}

void oscillator_set_freq(struct Oscillator *osc, float freq) {
	osc->freq = freq;
	osc->period = (float)SAMPLE_RATE / freq;
}
