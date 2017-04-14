#include "oscillator.h"

int16_t oscillator_render_sample(struct Oscillator *osc) {
	if(osc->period == 0) return 0;

	osc->phase += 0x10000; // add one sample in fixed
	osc->sub_phase += 0x10000;

	fixed sub_period = osc->period << 1;

	while(osc->phase > osc->period)
		osc->phase -= osc->period;
	while(osc->sub_phase > sub_period)
		osc->sub_phase -= sub_period;

	return (1 << 13) - (osc->phase << 3) / (osc->period >> 11) + (osc->sub_phase - osc->period / 2 > osc->period ? 4096 : -4096);
}
