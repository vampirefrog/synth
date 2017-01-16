#ifndef SYNTH_H_
#define SYNTH_H_

#include <stdint.h>

#define SYNTH_NUM_VOICES 64
#define TUNING 440

struct Voice {
	uint32_t phase;
	uint32_t time;
	uint16_t volume;
	uint32_t freq;

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
};

struct Synth {
	struct Voice voices[SYNTH_NUM_VOICES];
	uint16_t attack, decay, sustain, release; // in ms
};

void synth_init(struct Synth *synth);
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity);
void synth_note_off(struct Synth *synth, uint8_t note, uint8_t velocity);
int16_t synth_render_sample(struct Synth *synth);

#endif /* SYNTH_H_ */
