#include <math.h>

#include "filter.h"

void filter_init(struct Filter *filter, float cutoff, float resonance) {
	filter->v0 = filter->v1 = 0;
	filter->cutoff = cutoff;
	filter->resonance = resonance;
}

void filter_set_cutoff(struct Filter *filter, float cutoff) {
	filter->cutoff = cutoff;
}

void filter_set_resonance(struct Filter *filter, float resonance) {
	filter->resonance = resonance;
}

float filter_sample(struct Filter *filter, float input) {
	float c = pow(0.5, (128-filter->cutoff)   / 16.0);
	float r = pow(0.5, (filter->resonance+24) / 16.0);

	float outputs[2]; // oversample
	for(int i = 0; i < 2; i++) {
		filter->v0 =  (1-r*c)*filter->v0  -  (c)*filter->v1  + (c)*input;
		outputs[i] = filter->v1 = (1-r*c)*filter->v1  +  (c)*filter->v0;
	}

	return (outputs[0] + outputs[1]) / 2;
}
