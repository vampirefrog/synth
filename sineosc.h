#ifndef SINEOSC_H_
#define SINEOSC_H_

struct SineOsc {
	float a, s0, s1;
};

void sine_osc_init(struct SineOsc *osc);
void sine_osc_set_freq(struct SineOsc *osc, float freq);
float sine_osc_sample(struct SineOsc *osc);

#endif /* SINEOSC_H_ */
