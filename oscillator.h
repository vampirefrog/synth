#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "types.h"

struct Oscillator {
	fixed phase, sub_phase; // fixed 16.16
	fixed freq; // fixed 16.16
	fixed period; // period in samples, fixed 16.16

	enum {
		Rectangle,
		Triangle
	} type;

	uint8_t sub_oct; // 0-2
	uint8_t pulse_width; // 0 - 127
};

int16_t oscillator_render_sample(struct Oscillator *osc);
void oscillator_set_freq(fixed freq); // fixed 16.16

#endif /* OSCILLATOR_H_ */
