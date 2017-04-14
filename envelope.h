#ifndef ENVELOPE_H_
#define ENVELOPE_H_

#include <stdint.h>

struct Envelope {
	// public
	uint16_t attack, decay, release; // in ms
	uint16_t sustain; // percentage

	// private
	enum {
		None,
		Attack,
		Decay,
		Sustain,
		Release
	} state;

	uint32_t time;
};
void envelope_start(struct Envelope *env);
void envelope_stop(struct Envelope *env);
float envelope_sample(struct Envelope *env);

#endif /* ENVELOPE_H_ */
