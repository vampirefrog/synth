#ifndef FILTER_H_
#define FILTER_H_

struct Filter {
	float v0, v1;
	float cutoff, resonance;
};
void filter_init(struct Filter *filter, float cutoff, float q);
void filter_set_cutoff(struct Filter *filter, float cutoff); // frequency in Hz
void filter_set_resonance(struct Filter *filter, float q);
void filter_set_cutoff_and_resonance(struct Filter *filter, float cutoff, float q); // both
float filter_sample(struct Filter *filter, float input);

#endif /* FILTER_H_ */
