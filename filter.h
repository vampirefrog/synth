#ifndef FILTER_H_
#define FILTER_H_

struct Filter {
	// public
	float cutoff, resonance;

	// private
	float resonance_input, stage_1, stage_2, stage_3, stage_4;
	float cutoff_calc, resonance_calc;
	int last_calc;

	float b0, b1, b2, b3, b4;  //filter buffers (beware denormals!)
};
void filter_init(struct Filter *filter, float cutoff, float q);
void filter_set_cutoff(struct Filter *filter, float cutoff);
void filter_set_resonance(struct Filter *filter, float resonance);
float filter_sample(struct Filter *filter, float input);

#endif /* FILTER_H_ */
