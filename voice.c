#include "synth.h"

void voice_init(struct Voice *v, struct Synth *synth) {
	v->synth = synth;
	envelope_init(&v->osc_env);
	envelope_init(&v->filter_env);
}

void voice_render_sample(struct Voice *v, float *out) {
	float amplitude = envelope_sample(&v->osc_env); // calculate based on key velocity and envelope
	float cutoff = envelope_sample(&v->filter_env);

	float s = oscillator_render_sample(&v->osc);
	filter_set_cutoff(&v->filter, v->synth->cutoff + cutoff * v->synth->filter_eg_intensity);
	float f = filter_sample(&v->filter, v->volume * amplitude * s / 32768.0);
	out[0] = (1 + v->pan) * f / 2;
	out[1] = (1 - v->pan) * f / 2;
	v->time++;
}

void voice_note_start(struct Voice *voice, uint8_t note, float velocity) {
	voice->volume = velocity; // in the range (0, 1]

	envelope_start(&voice->osc_env);
	envelope_start(&voice->filter_env);

	note &= 0x7f;
	voice->time = 0;
	voice->note = note;
	voice->osc.sub_phase = voice->osc.phase;
	voice->osc.sub_oct = 1;
}

void voice_stop(struct Voice *voice) {
	envelope_stop(&voice->osc_env);
	envelope_stop(&voice->filter_env);
}
