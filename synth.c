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
	synth_set_cutoff_freq(synth, 100);
	synth_set_resonance(synth, 1);
}

uint8_t midi_notes[128];
void synth_note_on(struct Synth *synth, uint8_t note, uint8_t velocity) {
	// find the first available voice, if any
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *voice = synth->voices + i;
		if(voice->env_state == None) {
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
	struct Voice *voice = synth->voices + midi_notes[note & 0x7f];
	voice_stop(voice);
}

void synth_render_sample(struct Synth *synth, int16_t *out) {
	int32_t smpl[2] = { 0, 0 };
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = synth->voices + i;
		if(v->env_state == None)
			continue;
		int16_t vsmpl[2];
		voice_render_sample(v, vsmpl);
		for(int j = 0; j < 2; j++) {
			smpl[j] += vsmpl[j];
			if(smpl[j] > 32767) smpl[j] = 32767;
			if(smpl[j] < -32768) smpl[j] = -32768;
		}
	}
	out[0] = smpl[0];
	out[1] = smpl[1];
}

void synth_set_pulse_width(struct Synth *synth, uint8_t w) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		for(int j = 0; j < 7; j++)
			synth->voices[i].osc[j].pulse_width = w;
	}
}

void synth_set_cutoff_freq(struct Synth *synth, uint8_t f) {
	float freq = 20.0 + pow(2, f * 14.0 / 127.0);
	//float freq = 20 + 20000*sqrt(f/127.0);
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		filter_set_cutoff(&synth->voices[i].filter[0], freq);
		filter_set_cutoff(&synth->voices[i].filter[1], freq);
	}
}

void synth_set_resonance(struct Synth *synth, uint8_t f) {
	float q = 1 + f / 100.0;
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		filter_set_q(&synth->voices[i].filter[0], q);
		filter_set_q(&synth->voices[i].filter[1], q);
	}
}

void voice_calc_unison(struct Voice *voice);
void synth_set_unison_spread(struct Synth *synth, uint8_t w) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = synth->voices + i;
		v->unison_spread = w;
		if(v->env_state == None)
			continue;
		voice_calc_unison(v);
	}
}

void voice_calc_stereo_spread(struct Voice *voice);
void synth_set_stereo_spread(struct Synth *synth, uint8_t w) {
	for(int i = 0; i < SYNTH_NUM_VOICES; i++) {
		struct Voice *v = synth->voices + i;
		v->stereo_spread = w;
		if(v->env_state == None)
			continue;
		voice_calc_stereo_spread(v);
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
			return -(1 << 11);
		else
			return (1 << 11) - 1;
	} else if(osc->type == Triangle) {
		if(osc->phase < edge) {
			return (osc->phase << 3) / (edge >> 11) - (1 << 13);
		} else if(osc->pulse_width < 127) {
			return ((osc->period - osc->phase) << 3) / ((osc->period - edge) >> 11) - (1 << 13);
		} else {
			return 0;
		}
	}
}

void voice_render_sample(struct Voice *v, int16_t *out) {
//	if(v->env_state == None)
//		return 0;

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

	int32_t ret[2] = { 0, 0 };
	for(int i = 0; i < 7; i++) {
		int32_t s = oscillator_render_sample(&v->osc[i]);
		ret[0] +=  (127 + v->osc[i].pan) * s / 255;
		ret[1] += (127 - v->osc[i].pan) * s / 255;
	}
	out[0] = filter_sample(&v->filter[0], amplitude * ret[0] / 32767.0);
	out[1] = filter_sample(&v->filter[1], amplitude * ret[1] / 32767.0);
}

void voice_calc_unison(struct Voice *voice) {
	voice->osc[0].freq = voice->freq;
	int freq_below = voice->osc[0].freq - midi_note_freq_fixed[voice->note - 1];
	int freq_above = midi_note_freq_fixed[voice->note + 1] - voice->osc[0].freq;
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

void voice_calc_stereo_spread(struct Voice *voice) {
	voice->osc[0].pan = 0;
	for(int i = 1; i <= 7; i++) {
		voice->osc[i].pan = ((i&1) * 2 - 1) * voice->stereo_spread * ((i + 1) / 2) / 3;
	}
	printf(
		"Stereo spread %d -> %d  %d %d %d  %d %d %d\n",
		voice->stereo_spread,
		voice->osc[0].pan,
		voice->osc[1].pan,
		voice->osc[2].pan,
		voice->osc[3].pan,
		voice->osc[4].pan,
		voice->osc[5].pan,
		voice->osc[6].pan
	);
}

void voice_note_start(struct Voice *voice, uint8_t note, uint8_t velocity) {
	voice->time = 0;
	voice->volume = velocity * 128;
	voice->env_state = Attack;

	note &= 0x7f;
	voice->osc[0].type = Triangle;
	voice->freq = midi_note_freq_fixed[note];
	voice->note = note;
	for(int i = 0; i < 7; i++) voice->osc[0].phase = 0;
	voice_calc_unison(voice);
}

void voice_stop(struct Voice *voice) {
	voice->time = 0;
	voice->env_state = Release;
}


struct Biquad {
	double a0, a1, a2;
	double b0, b1, b2;
};
static struct Biquad ProtoCoef[IIR_LENGTH] = {{
	.a0 = 1.0,
	.a1 = 0,
	.a2 = 0,
	.b0 = 1.0,
	.b1 = 0.765367,
	.b2 = 1.0
}, {
	.a0 = 1.0,
	.a1 = 0,
	.a2 = 0,
	.b0 = 1.0,
	.b1 = 1.847759,
	.b2 = 1.0
}};

void prewarp(double *a0, double *a1, double *a2, double fc, double fs) {
	double wp;

	wp = 2.0 * fs * tan(M_PI * fc / fs);

	*a2 = (*a2) / (wp * wp);
	*a1 = (*a1) / wp;
}

void bilinear(
	double a0, double a1, double a2,    /* numerator coefficients */
	double b0, double b1, double b2,    /* denominator coefficients */
	double *k,           /* overall gain factor */
	double fs,           /* sampling rate */
	float *coef         /* pointer to 4 iir coefficients */
) {
	double ad, bd;

				 /* alpha (Numerator in s-domain) */
	ad = 4. * a2 * fs * fs + 2. * a1 * fs + a0;
				 /* beta (Denominator in s-domain) */
	bd = 4. * b2 * fs * fs + 2. * b1* fs + b0;
				 /* update gain constant for this section */
	*k *= ad/bd;

				 /* Denominator */
	*coef++ = (2. * b0 - 8. * b2 * fs * fs)
						   / bd;         /* beta1 */
	*coef++ = (4. * b2 * fs * fs - 2. * b1 * fs + b0)
						   / bd; /* beta2 */

				 /* Nominator */
	*coef++ = (2. * a0 - 8. * a2 * fs * fs)
						   / ad;         /* alpha1 */
	*coef = (4. * a2 * fs * fs - 2. * a1 * fs + a0)
						   / ad;   /* alpha2 */
}


/*
 * ----------------------------------------------------------
 * Transform from s to z domain using bilinear transform
 * with prewarp.
 *
 * Arguments:
 *      For argument description look at bilinear()
 *
 *      coef - pointer to array of floating point coefficients,
 *                     corresponding to output of bilinear transofrm
 *                     (z domain).
 *
 * Note: frequencies are in Hz.
 * ----------------------------------------------------------
 */
void szxform(
	double *a0, double *a1, double *a2, /* numerator coefficients */
	double *b0, double *b1, double *b2, /* denominator coefficients */
	double fc,         /* Filter cutoff frequency */
	double fs,         /* sampling rate */
	double *k,         /* overall gain factor */
	float *coef        /* pointer to 4 iir coefficients */
) {
		/* Calculate a1 and a2 and overwrite the original values */
		prewarp(a0, a1, a2, fc, fs);
		prewarp(b0, b1, b2, fc, fs);
		bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, fs, coef);
}

void filter_init(struct Filter *filter, float cutoff, float q) {
	filter_set_cutoff_q(filter, cutoff, q);
}

void filter_set_cutoff(struct Filter *filter, float cutoff) {
	filter_set_cutoff_q(filter, cutoff, filter->q);
}
void filter_set_q(struct Filter *filter, float q) {
	filter_set_cutoff_q(filter, filter->cutoff, q);
}
void filter_set_cutoff_q(struct Filter *filter, float cutoff, float q) {
	float    *coef;
	unsigned nInd;
	double   a0, a1, a2, b0, b1, b2;
	double   k;           /* overall gain factor */

	filter->cutoff = cutoff;
	filter->q = q;

	k = 1.0;                /* Set overall filter gain */
	coef = filter->coef + 1; /* Skip k, or gain */

	/*
	 * Compute z-domain coefficients for each biquad section
	 * for new Cutoff Frequency and Resonance
	 */
	for (nInd = 0; nInd < IIR_LENGTH; nInd++) {
		a0 = ProtoCoef[nInd].a0;
		a1 = ProtoCoef[nInd].a1;
		a2 = ProtoCoef[nInd].a2;

		b0 = ProtoCoef[nInd].b0;
		b1 = ProtoCoef[nInd].b1 / q;      /* Divide by resonance or Q */
		b2 = ProtoCoef[nInd].b2;
		szxform(&a0, &a1, &a2, &b0, &b1, &b2, cutoff, SAMPLE_RATE, &k, coef);
		coef += 4;                       /* Point to next filter section */
	}

	/* Update overall filter gain in coef array */
	filter->coef[0] = k;

	printf("%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
		cutoff, q,
		filter->coef[0],
		filter->coef[1], filter->coef[2], filter->coef[3],
		filter->coef[4], filter->coef[5], filter->coef[6]);
}

float filter_sample(struct Filter *filter, float input) {
	unsigned int i;
	float *hist1_ptr, *hist2_ptr, *coef_ptr;
	float output, new_hist, history1, history2;

	/* allocate history array if different size than last call */

	coef_ptr = filter->coef;                /* coefficient pointer */

	hist1_ptr = filter->history;            /* first history */
	hist2_ptr = hist1_ptr + 1;           /* next history */

	/* 1st number of coefficients array is overall input scale factor,
	 * or filter gain */
	output = input * (*coef_ptr++);

	for (i = 0 ; i < IIR_LENGTH; i++) {
		history1 = *hist1_ptr;           /* history values */
		history2 = *hist2_ptr;

		output = output - history1 * (*coef_ptr++);
		new_hist = output - history2 * (*coef_ptr++);    /* poles */

		output = new_hist + history1 * (*coef_ptr++);
		output = output + history2 * (*coef_ptr++);      /* zeros */

		*hist2_ptr++ = *hist1_ptr;
		*hist1_ptr++ = new_hist;
		hist1_ptr++;
		hist2_ptr++;
	}

	return output;
}
