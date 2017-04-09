#ifndef SYNTH_H_
#define SYNTH_H_

#include <stdint.h>

#define SYNTH_NUM_VOICES 64
#define TUNING 440

typedef uint32_t fixed;

struct Oscillator {
	fixed phase; // fixed 16.16
	fixed freq; // fixed 16.16
	fixed period; // period in samples, fixed 16.16

	enum {
		Rectangle,
		Triangle
	} type;

	uint8_t pulse_width; // 0 - 127
};

int16_t oscillator_render_sample(struct Oscillator *osc);
void oscillator_set_freq(fixed freq); // fixed 16.16

struct Sampler {
	fixed phase; // fixed 16.16
	fixed freq; // fixed 16.16
	fixed period; // period in samples, fixed 16.16

	uint32_t length;
	uint16_t length_log2; // length = 1 << length_log2;
	int16_t *data;
	enum {
		Nearest,
		Linear
	} interpolation;
};

int16_t sampler_sample(struct Sampler *sampler);
void sampler_set_freq(fixed freq);

#define IIR_LENGTH 2
struct Filter {
	float history[IIR_LENGTH * 2];
	float coef[4 * IIR_LENGTH + 1];
	float cutoff, q;
};
void filter_init(struct Filter *filter, float cutoff, float q);
void filter_set_cutoff(struct Filter *filter, float cutoff); // frequency in Hz
void filter_set_q(struct Filter *filter, float q);
void filter_set_cutoff_q(struct Filter *filter, float cutoff, float q); // both
float filter_sample(struct Filter *filter, float input);

struct Voice {
	uint32_t time;
	uint16_t volume;

	// Envelope
	enum {
		None,
		Attack,
		Decay,
		Sustain,
		Release
	} env_state;

	uint16_t attack, decay, release; // in ms
	uint16_t sustain; // percentage

	fixed freq;
	uint8_t note;

	uint8_t unison_spread;
	struct Oscillator osc[7];

	struct Filter filter;
};

struct Synth;
void voice_note_start(struct Voice *voice, uint8_t note, uint8_t velocity);
void voice_stop(struct Voice *voice);
int16_t voice_render_sample(struct Voice *voice);

struct Synth {
	struct Voice voices[SYNTH_NUM_VOICES];
	uint16_t attack, decay, sustain, release; // in ms
};

void synth_init(struct Synth *synth);
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity);
void synth_note_off(struct Synth *synth, uint8_t note, uint8_t velocity);
int16_t synth_render_sample(struct Synth *synth);
void synth_set_pulse_width(struct Synth *s, uint8_t width);
void synth_set_unison_spread(struct Synth *s, uint8_t spread);
void synth_set_cutoff_freq(struct Synth *s, uint8_t freq);
void synth_set_resonance(struct Synth *s, uint8_t freq);

#endif /* SYNTH_H_ */
