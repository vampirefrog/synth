#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "filter.h"

void filter_init(struct Filter *filter, float cutoff, float resonance) {
	memset(filter, 0, sizeof(struct Filter));
	filter->cutoff = cutoff;
	filter->resonance = resonance;
}

void filter_set_cutoff(struct Filter *filter, float cutoff) {
	filter->cutoff = cutoff;
	if(filter->cutoff > 0.9) filter->cutoff = 0.9;
	else if(filter->cutoff < 0.0) filter->cutoff = 0.0;
}

void filter_set_resonance(struct Filter *filter, float resonance) {
	filter->resonance = resonance;
}

// http://www.musicdsp.org/showone.php?id=25
// float filter_sample(struct Filter *filter, float input) {
// 	float q = 1.0f - filter->cutoff;
// 	float p = filter->cutoff + 0.8f * filter->cutoff * q;
// 	float f = p + p - 1.0f;
// 	q = filter->resonance * (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));

// #define OVERSAMPLE 2
// 	float output = 0;
// 	for(int i = 0; i < OVERSAMPLE; i++) {
// 		input -= q * filter->b4;                          //feedback
// 		float t1 = filter->b1;  filter->b1 = (input + filter->b0) * p - filter->b1 * f;
// 		float t2 = filter->b2;  filter->b2 = (filter->b1 + t1) * p - filter->b2 * f;
// 		t1 = filter->b3;  filter->b3 = (filter->b2 + t2) * p - filter->b3 * f;
// 		filter->b4 = (filter->b3 + t1) * p - filter->b4 * f;
// 		filter->b4 = filter->b4 - filter->b4 * filter->b4 * filter->b4 * 0.166667f;    //clipping
// 		filter->b0 = input;
// 		output += filter->b4;
// 	}
// 	return output / (float)OVERSAMPLE;
// }

// mad's filter code
float filter_sample(struct Filter *filter, float input) {
	if(++filter->last_calc >= 16) {
		filter->last_calc = 0;
		filter->cutoff_calc = pow (0.5, 8.5 - (filter->cutoff)*8  );
		filter->resonance_calc = filter->resonance;
	}
	float cutoff = filter->cutoff_calc;
	float resonance = filter->resonance_calc;

#define OVERSAMPLE 2
	float output = 0;
	for(int i = 0; i < OVERSAMPLE; i++) {
		float in = filter->resonance_input;
		if(in > 1) in = 1;
		if(in < -1) in = -1;
		in = 1.5*in - 0.5*in*in*in;
		in = input - in;
		filter->stage_1 += (in - filter->stage_1) * cutoff;
		filter->stage_2 += (filter->stage_1 - filter->stage_2) * cutoff;
		filter->stage_3 += (filter->stage_2 - filter->stage_3) * cutoff;
		filter->stage_4 += (filter->stage_3 - filter->stage_4) * cutoff;
		output += filter->stage_4;
		filter->resonance_input = output * resonance;
	}

	return output / OVERSAMPLE;
}
