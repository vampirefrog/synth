#include <stdio.h>
#include <string.h>

#include "synth.h"
#include "config.h"

#include "tables.inc"

// convert from 16.16 frequency to 16.16 period in samples
static fixed freq2period(fixed freq) {
	uint64_t tmp = (uint64_t)SAMPLE_RATE << 32;
	return tmp / freq;
}

void synth_init(struct Synth *synth) {
	synth_set_cutoff_freq(synth, 127);
	synth_set_resonance(synth, 0);
}

uint8_t midi_notes[128];
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity) {
	if(synth->monophonic) {
		int freq = midi_note_freq_fixed[note];
		int freq_below = freq - midi_note_freq_fixed[note - 1];
		int freq_above = midi_note_freq_fixed[note + 1] - freq;
		synth->voices[0].osc.freq = freq;
		synth->voices[0].osc.period = freq2period(freq);
		for(int i = 1; i <= 3; i++) {
			struct Voice *voice = synth->voices + i;
			voice->osc.freq = freq - synth->unison_spread * freq_below * i / 512;
			voice->osc.period = freq2period(voice->osc.freq);
		}
		for(int i = 4; i < 7; i++) {
			struct Voice *voice = synth->voices + i;
			voice->osc.freq = freq + synth->unison_spread * freq_above * (i - 3) / 512;
			voice->osc.period = freq2period(voice->osc.freq);
		}

		synth->voices[0].pan = 0;
		for(int i = 1; i <= 7; i++) {
			synth->voices[i].pan = ((i&1) * 2 - 1) * synth->stereo_spread * ((i + 1) / 2) / 3;
		}

		for(int i = 0; i < 7; i++) {
			struct Voice *voice = synth->voices + i;
			voice->osc.phase = random();

			voice->osc_env.attack = synth->osc_env.attack;
			voice->osc_env.decay = synth->osc_env.decay;
			voice->osc_env.sustain = synth->osc_env.sustain;
			voice->osc_env.release = synth->osc_env.release;

			voice->filter_env.attack = synth->filter_env.attack;
			voice->filter_env.decay = synth->filter_env.decay;
			voice->filter_env.sustain = synth->filter_env.sustain;
			voice->filter_env.release = synth->filter_env.release;

			voice_note_start(voice, note, i == 0 ? velocity : velocity / 4);
		}
	}
}

void synth_note_off(struct Synth *synth, uint8_t note, uint8_t velocity) {
	(void)velocity;
	for(int i = 0; i < 7; i++) {
		struct Voice *voice = synth->voices + i;
		voice_stop(voice);
	}
}

void synth_render_sample(struct Synth *synth, float *out) {
	float smpl[2] = { 0, 0 };

	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = synth->voices + i;
		float vsmpl[2];
		voice_render_sample(v, vsmpl);
		for(int j = 0; j < 2; j++) {
			smpl[j] += vsmpl[j] / 2.0;
			if(smpl[j] > 1) smpl[j] = 1;
			else if(smpl[j] < -1) smpl[j] = -1;
		}
	}

	out[0] = smpl[0];
	out[1] = smpl[1];
}

void synth_set_cutoff_freq(struct Synth *synth, uint8_t f) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		filter_set_cutoff(&synth->voices[i].filter, f);
	}
}

void synth_set_resonance(struct Synth *synth, uint8_t f) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		filter_set_resonance(&synth->voices[i].filter, f);
	}
}

void synth_set_unison_spread(struct Synth *synth, uint8_t w) {
	synth->unison_spread = w;
}

void synth_set_stereo_spread(struct Synth *synth, uint8_t w) {
	synth->stereo_spread = w;
}
