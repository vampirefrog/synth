#ifndef VOICE_H_
#define VOICE_H_

#include "types.h"
#include "envelope.h"
#include "oscillator.h"
#include "filter.h"

struct Synth;
struct Voice {
	struct Synth *synth;

	uint8_t note;
	float detune;
	float volume;
	float pan;
	uint32_t time;

	struct Envelope osc_env, filter_env;
	struct Oscillator osc;
	struct Filter filter;
};

void voice_init(struct Voice *voice, struct Synth *synth);
void voice_note_start(struct Voice *voice, uint8_t note, float velocity);
void voice_stop(struct Voice *voice);
void voice_render_sample(struct Voice *voice, float *out);

#endif /* VOICE_H_ */
