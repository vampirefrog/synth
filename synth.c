#include <stdio.h>
#include <math.h>

#include "synth.h"
#include "config.h"

#include "tables.inc"

// convert from 16.16 frequency to 16.16 period in samples
static fixed freq2period(fixed freq) {
	uint64_t tmp = (uint64_t)SAMPLE_RATE << 32;
	return tmp / freq;
}

void synth_init(struct Synth *synth) {
}

uint8_t midi_notes[128];
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity) {
	printf("synth_note_on note=%d velocity=%d\n", note, velocity);
	// find the first available voice, if any
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *voice = synth->voices + i;
		if(voice->env_state == None) {
			printf("starting voice %d\n", i);
			midi_notes[note & 0x7f] = i;
			voice->attack = synth->attack;
			voice->decay = synth->decay;
			voice->sustain = synth->sustain;
			voice->release = synth->release;
			voice_note_start(voice, note, velocity);
			break;
		}
	}
}

void synth_note_off(struct Synth *synth, uint8_t note, uint8_t velocity) {
	(void)velocity;
	printf("synth_note_off note=%d velocity=%d\n", note, velocity);
	printf("stopping voice %d\n", note & 0x7f);
	struct Voice *voice = synth->voices + midi_notes[note & 0x7f];
	voice_stop(voice);
}

static uint8_t alt = 0;
int16_t synth_render_sample(struct Synth *synth) {
	int32_t smpl = 0;
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = synth->voices + i;
		smpl += voice_render_sample(v);
		if(smpl > 32767) smpl = 32767;
		if(smpl < -32768) smpl = -32768;
	}
	alt = (alt+1)&1;
	return smpl & 0xffff;
}

void synth_set_pulse_width(struct Synth *synth, uint8_t w) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		for(int j = 0; j < 7; j++)
			synth->voices[i].osc[j].pulse_width = w;
	}
}

void synth_set_unison_spread(struct Synth *synth, uint8_t w) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		for(int j = 0; j < 7; j++)
			synth->voices[i].unison_spread = w;
	}
}

int16_t oscillator_render_sample(struct Oscillator *osc) {
	osc->phase += 0x10000; // add one sample in fixed
	if(osc->period > 0) // avoid a infinite loop below
		while(osc->phase > osc->period)
			osc->phase -= osc->period;
	uint32_t edge = osc->pulse_width * osc->period / 127;
	if(osc->type == Rectangle) {
		if(osc->phase >= edge)
			return -20000;
		else
			return 20000;
	} else if(osc->type == Triangle) {
		if(osc->phase <= edge) {
			return ((((osc->phase >> 3) << 6) / (edge >> 3)) << 6) - (1 << 11);
		} else {
			return (((((osc->period - osc->phase) >> 3) << 6) / ((osc->period - edge) >> 3)) << 6) - (1 << 11);
		}
	}
}

int16_t voice_render_sample(struct Voice *v) {
	if(v->env_state == None)
		return 0;

	int32_t amplitude = 0; // calculate based on velocity and envelope
	uint32_t ms_time = v->time * 1000 / SAMPLE_RATE;
	switch(v->env_state) {
		case Attack:
			if(v->attack > 0)
				amplitude = v->volume * ms_time / v->attack;
			if(ms_time > v->attack) {
				v->env_state = Decay;
				v->time = 0;
			}
			break;
		case Decay:
			if(v->decay > 0)
				amplitude = (100 * v->volume * (v->decay - ms_time) + v->sustain * v->volume * ms_time) / v->decay / 100;
			if(ms_time > v->decay) {
				v->env_state = Sustain;
				v->time = 0;
			}
			break;
		case Sustain:
			amplitude = v->volume * v->sustain / 100;
			break;
		case Release:
			if(v->release > 0)
				amplitude = v->volume * (v->release - ms_time) * v->sustain / v->release / 100;
			if(ms_time > v->release) {
				v->time = 0;
				v->env_state = None;
			}
			break;
	}

	v->time++;

	if(v->env_state != None) {
		int32_t ret = 0;
		for(int i = 0; i < 7; i++) ret += oscillator_render_sample(&v->osc[i]);
		return amplitude * ret / 32767;
	}
}

void voice_note_start(struct Voice *voice, uint8_t note, uint8_t velocity) {
	voice->time = 0;
	voice->volume = velocity * 128;
	voice->env_state = Attack;

	note &= 0x7f;
	voice->osc[0].type = Triangle;
	voice->osc[0].freq = midi_note_freq_fixed[note];
	int freq_below = voice->osc[0].freq - midi_note_freq_fixed[note - 1];
	int freq_above = midi_note_freq_fixed[note + 1] - voice->osc[0].freq;
	voice->osc[0].period = freq2period(voice->osc[0].freq);
	for(int i = 1; i <= 3; i++) {
		voice->osc[i].type = voice->osc[0].type;
		voice->osc[i].freq = voice->osc[0].freq - voice->unison_spread * freq_below * i / 3 / 127;
		voice->osc[i].period = freq2period(voice->osc[i].freq);
	}
	for(int i = 4; i < 7; i++) {
		voice->osc[i].type = voice->osc[0].type;
		voice->osc[i].freq = voice->osc[0].freq + voice->unison_spread * freq_above * (i - 3) / 3 / 127;
		voice->osc[i].period = freq2period(voice->osc[i].freq);
	}
}

void voice_stop(struct Voice *voice) {
	voice->time = 0;
	voice->env_state = Release;
}
