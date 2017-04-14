#ifndef SYNTH_H_
#define SYNTH_H_

#include <stdint.h>
typedef uint32_t fixed;

#define SYNTH_NUM_VOICES 16
#define TUNING 440

#include "envelope.h"
#include "filter.h"
#include "oscillator.h"
#include "voice.h"

struct Synth {
	struct Voice voices[SYNTH_NUM_VOICES];
	struct Envelope osc_env, filter_env;
	uint8_t unison_spread;
	uint8_t stereo_spread;
	uint8_t monophonic;
};

void synth_init(struct Synth *synth);
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity);
void synth_note_off(struct Synth *synth, uint8_t note, uint8_t velocity);
void synth_render_sample(struct Synth *synth, float *out);
void synth_set_pulse_width(struct Synth *s, uint8_t width);
void synth_set_unison_spread(struct Synth *s, uint8_t spread);
void synth_set_stereo_spread(struct Synth *s, uint8_t spread);
void synth_set_cutoff_freq(struct Synth *s, uint8_t freq);
void synth_set_resonance(struct Synth *s, uint8_t freq);

#endif /* SYNTH_H_ */
