#include "midi.h"

void synth_midi_note_on(struct Synth *synth, uint8_t note, uint8_t velocity) {
	if(velocity == 0)
		synth_note_off(synth, note, 0);
	else
		synth_note_on(synth, note, velocity / 127.0);
}

void synth_midi_note_off(struct Synth *synth, uint8_t note, uint8_t velocity) {
	synth_note_off(synth, note, velocity / 127.0);
}

void synth_midi_pitch_bend(struct Synth *synth, int16_t value) {
	synth_pitch_bend(synth, (value - 8192) / 8191.0);
}

void synth_midi_cc(struct Synth *synth, uint8_t param, uint8_t value) {
	switch(param) {
		case 1:
			synth_set_lfo_depth(synth, value / 127.0);
			break;
		case 7:
			synth_set_volume(synth, value / 127.0);
			break;
		case 91:
			synth_set_unison_spread(synth, value / 127.0);
			break;
		case 93:
			synth_set_stereo_spread(synth, value / 127.0);
			break;
		case 74:
			synth_set_cutoff_freq(synth, value / 127.0);
			break;
		case 71:
			synth_set_resonance(synth, value / 127.0);
			break;
	}
}
