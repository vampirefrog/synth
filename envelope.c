#include "envelope.h"
#include "config.h"

void envelope_start(struct Envelope *env) {
	env->state = Attack;
	env->time = 0;
}

void envelope_stop(struct Envelope *env) {
	env->state = Release;
	env->time = 0;
}

float envelope_sample(struct Envelope *env) {
	if(env->state == None) return 0;

	float ret = 0;

	uint32_t ms_time = env->time * 1000 / SAMPLE_RATE;
	switch(env->state) {
		case Attack:
			if(ms_time > env->attack) {
				env->state = Decay;
				env->time = 0;
			} else if(env->attack > 0) {
				ret = (float)ms_time / (float)env->attack;
			}
			break;
		case Decay:
			if(ms_time > env->decay) {
				env->state = Sustain;
				env->time = 0;
			} else if(env->decay > 0) {
				ret = ((env->decay - ms_time) + env->sustain * ms_time / 100.0) / (float)env->decay;
			}
			break;
		case Sustain:
			ret = (float)env->sustain / 100.0;
			break;
		case Release:
			if(ms_time > env->release) {
				env->state = None;
				env->time = 0;
			} else if(env->release > 0) {
				ret = (env->release - ms_time) * env->sustain / env->release / 100.0;
			}
			break;
	}

	env->time++;

	return ret;
}
