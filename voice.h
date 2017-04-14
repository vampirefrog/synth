#ifndef VOICE_H_
#define VOICE_H_

#include "types.h"
#include "envelope.h"
#include "oscillator.h"
#include "filter.h"

struct Voice {
	uint8_t note;
	float volume;
	int8_t pan;

	struct Envelope osc_env, filter_env;
	struct Oscillator osc;
	struct Filter filter;
};

void voice_note_start(struct Voice *voice, uint8_t note, uint8_t velocity);
void voice_stop(struct Voice *voice);
void voice_render_sample(struct Voice *voice, float *out);

#endif /* VOICE_H_ */