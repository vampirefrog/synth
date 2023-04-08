#ifndef MIDI_H_
#define MIDI_H_

#include <stdint.h>

#include "synth.h"

void synth_midi_note_on(struct Synth *synth, uint8_t note, uint8_t velocity);
void synth_midi_note_off(struct Synth *synth, uint8_t note, uint8_t velocity);
void synth_midi_pitch_bend(struct Synth *synth, int16_t value);
void synth_midi_cc(struct Synth *synth, uint8_t param, uint8_t value);

#endif /* MIDI_H_ */
