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
#include "sineosc.h"

struct Key {
	uint8_t note;
	float velocity;
};

struct Synth {
	struct Voice voices[SYNTH_NUM_VOICES];
	float osc_attack, osc_decay, osc_sustain, osc_release;
	float filter_attack, filter_decay, filter_sustain, filter_release;
	float unison_spread;
	float stereo_spread;
	uint8_t monophonic;

	float tuning; // 440.0Hz
	float pitch_bend_range;
	float cutoff, filter_eg_intensity, filter_kbd_track, lfo_freq;

	// private
	struct SineOsc lfo_osc;
	struct Key key_stack[SYNTH_NUM_VOICES];
	uint8_t key_stack_size;
	float volume, pitch_bend, lfo_depth;
	uint32_t time;
};

void synth_init(struct Synth *synth);
void synth_load_patch(struct Synth *synth, const char *filename);
void synth_note_on(struct Synth *synth, uint8_t note, float velocity);
void synth_note_off(struct Synth *synth, uint8_t note, float velocity);
void synth_render_sample(struct Synth *synth, float *out);
void synth_set_unison_spread(struct Synth *s, float spread);
void synth_set_stereo_spread(struct Synth *s, float spread);
void synth_set_cutoff_freq(struct Synth *s, float freq);
void synth_set_resonance(struct Synth *s, float res);
void synth_set_volume(struct Synth *s, float vol);
void synth_pitch_bend(struct Synth *s, float bend);
void synth_set_lfo_depth(struct Synth *s, float value);
void synth_set_osc_attack(struct Synth *s, float value);
void synth_set_osc_decay(struct Synth *s, float value);
void synth_set_osc_sustain(struct Synth *s, float value);
void synth_set_osc_release(struct Synth *s, float value);
void synth_set_filter_attack(struct Synth *s, float value);
void synth_set_filter_decay(struct Synth *s, float value);
void synth_set_filter_sustain(struct Synth *s, float value);
void synth_set_filter_release(struct Synth *s, float value);
void synth_set_filter_eg_intensity(struct Synth *s, float value);
void synth_set_filter_kbd_track(struct Synth *s, float value);
void synth_set_pitch_bend_range(struct Synth *s, float value);
void synth_set_monophonic(struct Synth *s, int value);

#endif /* SYNTH_H_ */
