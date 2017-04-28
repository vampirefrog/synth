#ifndef ENVELOPE_H_
#define ENVELOPE_H_

#include <stdint.h>

struct Envelope {
	// private
	enum {
		EnvNone,
		EnvAttack,
		EnvDecay,
		EnvSustain,
		EnvRelease
	} state;

	float output,
		attack_rate,
		decay_rate,
		release_rate,
		attack_coef,
		decay_coef,
		release_coef,
		sustain_level,
		target_ratio_a,
		target_ratio_dr,
		attack_base,
		decay_base,
		release_base;
};

void envelope_init(struct Envelope *env);
void envelope_reset(struct Envelope *env);
void envelope_start(struct Envelope *env);
void envelope_stop(struct Envelope *env);
void envelope_set_attack_rate(struct Envelope *env, float rate);
void envelope_set_decay_rate(struct Envelope *env, float rate);
void envelope_set_release_rate(struct Envelope *env, float rate);
void envelope_set_sustain_level(struct Envelope *env, float level);
void envelope_set_target_ratio_a(struct Envelope *env, float target_ratio);
void envelope_set_target_ratio_dr(struct Envelope *env, float target_ratio);
float envelope_sample(struct Envelope *env);

#endif /* ENVELOPE_H_ */
