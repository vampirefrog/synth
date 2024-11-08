#ifndef __EMSCRIPTEN__
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "synth.h"
#include "config.h"

#include "tables.inc"

void synth_init(struct Synth *synth) {
	synth_set_cutoff_freq(synth, 0);
	synth_set_resonance(synth, 0);
	srandom(1234);
	synth->key_stack_size = 0;
	synth->tuning = 440.0;
	synth->volume = 1.0;
	synth->pitch_bend = 0.0;
	synth->lfo_depth = 0.0;
	sine_osc_init(&synth->lfo_osc);
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		voice_init(&synth->voices[i], synth);
	}
}

#ifndef __EMSCRIPTEN__
struct Synth *synth_new() {
	struct Synth *s = malloc(sizeof(struct Synth));
	synth_init(s);
	return s;
}

void synth_free(struct Synth *synth) {
	return free(synth);
}
#endif

static float detune(float freq, float semitones) {
	return freq * pow(2, semitones / 12.0);
}

static float midifreq(uint8_t note) {
	return pow(2, (note - 69) / 12.0) * 440.0;
}

static void synth_set_pitch(struct Synth *synth) {
	float lfo_detune = synth->lfo_depth * sine_osc_sample(&synth->lfo_osc);
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *voice = &synth->voices[i];
		if(voice->osc_env.state != EnvNone)
			oscillator_set_freq(&voice->osc, detune(midifreq(voice->note), voice->detune + synth->pitch_bend * synth->pitch_bend_range + lfo_detune));
	}
}

static void synth_spread_unison(struct Synth *synth) {
	synth->voices[0].detune = 0;
	for(int i = 1; i <= 3; i++) {
		synth->voices[i].detune = i * synth->unison_spread / 6.0;
	}
	for(int i = 4; i < 7; i++) {
		synth->voices[i].detune = (i - 7) * synth->unison_spread / 6.0;
	}
}

static void synth_spread_stereo(struct Synth *synth) {
	synth->voices[0].pan = 0;
	for(int i = 1; i <= 7; i++) {
		synth->voices[i].pan = ((i&1) * 2 - 1) * synth->stereo_spread * ((i + 1) / 2) / 3;
	}
	// for(int i = 0; i < 7; i++) {
	// 	printf("voice %d pan %f\n", i, synth->voices[i].pan);
	// }
}

static void synth_note_on_monophonic(struct Synth *synth, uint8_t note, float velocity) {
	synth_spread_unison(synth);
	synth_spread_stereo(synth);

	for(int i = 0; i < 7; i++) {
		struct Voice *voice = synth->voices + i;
		voice->osc.phase = random();

		envelope_set_attack_rate(&voice->osc_env, synth->osc_attack);
		envelope_set_decay_rate(&voice->osc_env, synth->osc_decay);
		envelope_set_sustain_level(&voice->osc_env, synth->osc_sustain);
		envelope_set_release_rate(&voice->osc_env, synth->osc_release);

		envelope_set_attack_rate(&voice->filter_env, synth->filter_attack);
		envelope_set_decay_rate(&voice->filter_env, synth->filter_decay);
		envelope_set_sustain_level(&voice->filter_env, synth->filter_sustain);
		envelope_set_release_rate(&voice->filter_env, synth->filter_release);

		voice_note_start(voice, note, velocity);
	}
}

void synth_note_on(struct Synth *synth, uint8_t note, float velocity) {
	if(synth->monophonic) {
		synth->key_stack[synth->key_stack_size].note = note;
		synth->key_stack[synth->key_stack_size].velocity = velocity;
		synth->key_stack_size++;
		if(synth->key_stack_size >= SYNTH_NUM_VOICES) {
			memmove(synth->key_stack, synth->key_stack + 1, sizeof(synth->key_stack[0]) * (SYNTH_NUM_VOICES - 1));
			synth->key_stack_size--;
		}
		synth_note_on_monophonic(synth, note, velocity);
	} else {
		struct Voice *found = 0;
		uint32_t oldest_time = 0;
		for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
			struct Voice *voice = &synth->voices[i];
			if(voice->osc_env.state == EnvNone) {
				found = voice;
				break;
			}
			if(voice->time > oldest_time) {
				oldest_time = voice->time;
				found = voice;
			}
		}

		envelope_set_attack_rate(&found->osc_env, synth->osc_attack);
		envelope_set_decay_rate(&found->osc_env, synth->osc_decay);
		envelope_set_sustain_level(&found->osc_env, synth->osc_sustain);
		envelope_set_release_rate(&found->osc_env, synth->osc_release);

		envelope_set_attack_rate(&found->filter_env, synth->filter_attack);
		envelope_set_decay_rate(&found->filter_env, synth->filter_decay);
		envelope_set_sustain_level(&found->filter_env, synth->filter_sustain);
		envelope_set_release_rate(&found->filter_env, synth->filter_release);

		voice_note_start(found, note, velocity);
	}
}

static void synth_note_off_monophonic(struct Synth *synth) {
	for(int i = 0; i < 7; i++) {
		struct Voice *voice = synth->voices + i;
		voice_stop(voice);
	}
}

void synth_note_off(struct Synth *synth, uint8_t note, float velocity) {
	if(synth->monophonic) {
		if(synth->key_stack_size > 1) {
			if(synth->key_stack[synth->key_stack_size - 1].note == note) {
				synth_note_on_monophonic(synth, synth->key_stack[synth->key_stack_size - 2].note, synth->key_stack[synth->key_stack_size - 2].velocity);
			} else {
				for(int i = 0; i < synth->key_stack_size - 1; i++) {
					if(synth->key_stack[i].note == note) {
						memmove(synth->key_stack + i, synth->key_stack + i + 1, sizeof(synth->key_stack[0]) * (synth->key_stack_size - i - 1));
						break;
					}
				}
			}
		} else {
			synth_note_off_monophonic(synth);
		}
		synth->key_stack_size--;
	} else {
		for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
			struct Voice *voice = synth->voices + i;
			if(voice->note == note && voice->osc_env.state != EnvNone && voice->osc_env.state != EnvRelease) {
				voice_stop(&synth->voices[i]);
			}
		}
	}
}

void synth_render_sample(struct Synth *synth, float *out) {
	float smpl[2] = { 0, 0 };

	synth_set_pitch(synth);

	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = &synth->voices[i];

		float vsmpl[2];
		voice_render_sample(v, vsmpl);
		for(int j = 0; j < 2; j++) {
			smpl[j] += vsmpl[j] * synth->volume;
			if(smpl[j] > 1) smpl[j] = 1;
			else if(smpl[j] < -1) smpl[j] = -1;
		}
	}

	out[0] = smpl[0];
	out[1] = smpl[1];
	synth->time++;
}

void synth_render_buffer(struct Synth *synth, float *out, int num_samples) {
	for(int i = 0; i < num_samples; i++, out += 2) {
		synth_render_sample(synth, out);
	}
}

void synth_set_cutoff_freq(struct Synth *synth, float f) {
	synth->cutoff = f;
}

void synth_set_resonance(struct Synth *synth, float f) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		filter_set_resonance(&synth->voices[i].filter, f);
	}
}

void synth_set_unison_spread(struct Synth *synth, float w) {
	synth->unison_spread = w;
	synth_spread_unison(synth);
}

void synth_set_stereo_spread(struct Synth *synth, float w) {
	synth->stereo_spread = w;
	synth_spread_stereo(synth);
}

void synth_set_volume(struct Synth *s, float vol) {
	s->volume = vol;
}

void synth_pitch_bend(struct Synth *s, float bend) {
	s->pitch_bend = bend;
	if(s->pitch_bend < -1.0) s->pitch_bend = -1.0;
	if(s->pitch_bend > 1.0) s->pitch_bend = 1.0;
	synth_set_pitch(s);
}

void synth_set_lfo_depth(struct Synth *s, float depth) {
	s->lfo_depth = depth;
}

void synth_set_lfo_freq(struct Synth *s, float value) {
	sine_osc_set_freq(&s->lfo_osc, value);
}

void synth_set_osc_attack(struct Synth *s, float value) {
	s->osc_attack = value;
}

void synth_set_osc_decay(struct Synth *s, float value) {
	s->osc_decay = value;
}

void synth_set_osc_sustain(struct Synth *s, float value) {
	s->osc_sustain = value;
}

void synth_set_osc_release(struct Synth *s, float value) {
	s->osc_release = value;
}

void synth_set_filter_attack(struct Synth *s, float value) {
	s->filter_attack = value;
}

void synth_set_filter_decay(struct Synth *s, float value) {
	s->filter_decay = value;
}

void synth_set_filter_sustain(struct Synth *s, float value) {
	s->filter_sustain = value;
}

void synth_set_filter_release(struct Synth *s, float value) {
	s->filter_release = value;
}

void synth_set_filter_eg_intensity(struct Synth *s, float value) {
	s->filter_eg_intensity = value;
}

void synth_set_filter_kbd_track(struct Synth *s, float value) {
	s->filter_kbd_track = value;
}

void synth_set_pitch_bend_range(struct Synth *s, float value) {
	s->pitch_bend_range = value;
}

void synth_set_monophonic(struct Synth *s, int value) {
	s->monophonic = value;
}

#ifndef __EMSCRIPTEN__
void synth_load_patch(struct Synth *s, const char *filename) {
	FILE *f = fopen(filename, "r");
	if(!f) {
		perror(filename);
		return;
	}
	char buf[256];
	while(!feof(f)) {
		fgets(buf, sizeof(buf), f);
		char *tok = strtok(buf, " \t");
		if(tok) {
			char *val = strtok(NULL, " \t");
			if(val) {
				if(!strcmp(tok, "lfo_freq")) {
					synth_set_lfo_freq(s, strtof(val, NULL));
				} else if(!strcmp(tok, "osc_env.attack")) {
					synth_set_osc_attack(s, strtof(val, NULL));
				} else if(!strcmp(tok, "osc_env.decay")) {
					synth_set_osc_decay(s, strtof(val, NULL));
				} else if(!strcmp(tok, "osc_env.sustain")) {
					synth_set_osc_sustain(s, strtof(val, NULL));
				} else if(!strcmp(tok, "osc_env.release")) {
					synth_set_osc_release(s, strtof(val, NULL));
				} else if(!strcmp(tok, "filter_env.attack")) {
					synth_set_filter_attack(s, strtof(val, NULL));
				} else if(!strcmp(tok, "filter_env.decay")) {
					synth_set_filter_decay(s, strtof(val, NULL));
				} else if(!strcmp(tok, "filter_env.sustain")) {
					synth_set_filter_sustain(s, strtof(val, NULL));
				} else if(!strcmp(tok, "filter_env.release")) {
					synth_set_filter_release(s, strtof(val, NULL));
				} else if(!strcmp(tok, "filter_eg_intensity")) {
					synth_set_filter_eg_intensity(s,  strtof(val, NULL));
				} else if(!strcmp(tok, "filter_kbd_track")) {
					synth_set_filter_kbd_track(s, strtof(val, NULL));
				} else if(!strcmp(tok, "pitch_bend_range")) {
					synth_set_pitch_bend_range(s, strtof(val, NULL));
				} else if(!strcmp(tok, "monophonic")) {
					synth_set_monophonic(s, atoi(val));
				} else if(!strcmp(tok, "unison_spread")) {
					synth_set_unison_spread(s, strtof(val, NULL));
				} else if(!strcmp(tok, "stereo_spread")) {
					synth_set_stereo_spread(s, strtof(val, NULL));
				} else if(!strcmp(tok, "cutoff")) {
					synth_set_cutoff_freq(s, strtof(val, NULL));
				} else if(!strcmp(tok, "resonance")) {
					synth_set_resonance(s, strtof(val, NULL));
				}
			}
		}
	}
	fclose(f);
}
#endif
