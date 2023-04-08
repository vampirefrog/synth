#include <math.h>

#include "envelope.h"
#include "config.h"

void envelope_init(struct Envelope *env) {
	envelope_reset(env);
	envelope_set_attack_rate(env, 0);
	envelope_set_decay_rate(env, 0);
	envelope_set_release_rate(env, 0);
	envelope_set_sustain_level(env, 1.0);
	envelope_set_target_ratio_a(env, 0.3);
	envelope_set_target_ratio_dr(env, 0.0001);
}

void envelope_reset(struct Envelope *env) {
	env->state = EnvNone;
	env->output = 0.0;
}

void envelope_start(struct Envelope *env) {
	env->state = EnvAttack;
}

void envelope_stop(struct Envelope *env) {
	if (env->state != EnvNone)
		env->state = EnvRelease;
}

static inline float calc_coef(float rate, float target_ratio) {
	return (rate <= 0) ? 0.0 : exp(-log((1.0 + target_ratio) / target_ratio) / rate);
}

void envelope_set_attack_rate(struct Envelope *env, float rate) {
	env->attack_rate = rate;
	env->attack_coef = calc_coef(rate, env->target_ratio_a);
	env->attack_base = (1.0 + env->target_ratio_a) * (1.0 - env->attack_coef);
}

void envelope_set_decay_rate(struct Envelope *env, float rate) {
	env->decay_rate = rate;
	env->decay_coef = calc_coef(rate, env->target_ratio_dr);
	env->decay_base = (env->sustain_level - env->target_ratio_dr) * (1.0 - env->decay_coef);
}

void envelope_set_release_rate(struct Envelope *env, float rate) {
	env->release_rate = rate;
	env->release_coef = calc_coef(rate, env->target_ratio_dr);
	env->release_base = -env->target_ratio_dr * (1.0 - env->release_coef);
}

void envelope_set_sustain_level(struct Envelope *env, float level) {
	env->sustain_level = level;
	env->decay_base = (env->sustain_level - env->target_ratio_dr) * (1.0 - env->decay_coef);
}

void envelope_set_target_ratio_a(struct Envelope *env, float target_ratio) {
	if (target_ratio < 0.000000001)
		target_ratio = 0.000000001;  // -180 dB
	env->target_ratio_a = target_ratio;
	env->attack_coef = calc_coef(env->attack_rate, target_ratio);
	env->attack_base = (1.0 + env->target_ratio_a) * (1.0 - env->attack_coef);
}

void envelope_set_target_ratio_dr(struct Envelope *env, float target_ratio) {
	if (target_ratio < 0.000000001)
		target_ratio = 0.000000001;  // -180 dB
	env->target_ratio_dr = target_ratio;
	env->decay_coef = calc_coef(env->decay_rate, env->target_ratio_dr);
	env->release_coef = calc_coef(env->release_rate, env->target_ratio_dr);
	env->decay_base = (env->sustain_level - env->target_ratio_dr) * (1.0 - env->decay_coef);
	env->release_base = -env->target_ratio_dr * (1.0 - env->release_coef);
}

float envelope_sample(struct Envelope *env) {
	switch(env->state) {
		case EnvAttack:
			env->output = env->attack_base + env->output * env->attack_coef;
			if(env->output >= 1.0) {
				env->output = 1.0;
				env->state = EnvDecay;
			}
			break;
		case EnvDecay:
			env->output = env->decay_base + env->output * env->decay_coef;
			if(env->output <= env->sustain_level) {
				env->output = env->sustain_level;
				env->state = EnvSustain;
			}
			break;
		case EnvSustain:
			break;
		case EnvRelease:
			env->output = env->release_base + env->output * env->release_coef;
			if(env->output <= 0.0) {
				env->output = 0.0;
				env->state = EnvNone;
			}
			break;
	}

	return env->output;
}
