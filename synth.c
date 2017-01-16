#include <stdio.h>

#include "synth.h"
#include "config.h"

#include "tables.inc"

void synth_init(struct Synth *synth) {
}

uint8_t midi_notes[128];
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *voice = synth->voices + i;
		if(voice->env_state == None) {
			voice->phase = voice->time = 0;
			voice->freq = (uint32_t)(midi_note_freq_table[note & 0x7f] * 256);
			midi_notes[note & 0x7f] = i;
			voice->attack = synth->attack;
			voice->decay = synth->decay;
			voice->sustain = synth->sustain;
			voice->release = synth->release;
			voice->volume = velocity;
			voice->env_state = Attack;
			printf("Attack %d\n", synth->attack);
			break;
		}
	}
}

void synth_note_off(struct Synth *synth, uint8_t note, uint8_t velocity) {
	(void)velocity;
	struct Voice *voice = synth->voices + midi_notes[note & 0x7f];
	voice->time = 0;
	voice->env_state = Release;
	printf("Release %d\n", voice->release);
}

int16_t synth_render_sample(struct Synth *synth) {
	int32_t smpl = 0;
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = synth->voices + i;
		if(v->env_state != None) {

			v->time++;

			int32_t amplitude = 0; // calculate based on velocity and envelope
			uint32_t ms_time = v->time * 1000 / SAMPLE_RATE;
			switch(v->env_state) {
				case Attack:
					if(v->attack > 0)
						amplitude = v->volume * ms_time / v->attack;
					if(ms_time > v->attack) {
						printf("Decay %d\n", v->decay);
						v->env_state = Decay;
						v->time = 0;
					}
					break;
				case Decay:
					if(v->decay > 0)
						amplitude = (100 * v->volume * (v->decay - ms_time) + v->sustain * v->volume * ms_time) / v->decay / 100;
					if(ms_time > v->decay) {
						printf("Sustain %d\n", v->sustain);
						v->env_state = Sustain;
						v->time = 0;
					}
					break;
				case Sustain:
					amplitude = v->volume * v->sustain / 100;
					break;
				case Release:
					amplitude = v->volume * (v->release - ms_time) * v->sustain / v->release / 100;
					if(ms_time > v->release) {
						v->time = 0;
						v->freq = 0;
						v->env_state = None;
						printf("None\n");
					}
					break;
			}

			if(v->env_state != None && v->freq) {
				v->phase += (1 << 8);
				uint32_t freq_phase = (SAMPLE_RATE << 16) / v->freq;
				while(v->phase >= freq_phase)
					v->phase -= freq_phase;
				int sine_phase = 256 * v->phase / freq_phase;
				int sine_phase_next = (sine_phase + 1) & 0xff;
				int sine_remainder = v->phase % freq_phase;
				// if(v->env_state == Release) {
				// 	printf("amplitude %d volume=%d time=%d ms_time=%d v->release=%d v->sustain=%d\n", amplitude, v->volume, v->time, ms_time, v->release, v->sustain);
				// }
				smpl += amplitude * sine_table_256[sine_phase] / 255;
			}
			if(smpl > 32767) smpl = 32767;
			if(smpl < -32768) smpl = -32768;
		}
	}
	return smpl & 0xffff;
}
