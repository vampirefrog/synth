#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include <stdint.h>

struct Oscillator {
	float phase, sub_phase; // floating point phase
	float freq; // frequency in Hz
	float period; // period in samples (float)

	enum {
		Rectangle,
		Triangle
	} type;

	uint8_t sub_oct; // 0-2
	uint8_t pulse_width; // 0 - 127
};

int16_t oscillator_render_sample(struct Oscillator *osc);
void oscillator_set_freq(struct Oscillator *osc, float freq); // frequency in Hz

#endif /* OSCILLATOR_H_ */
