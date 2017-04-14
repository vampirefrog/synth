CFLAGS=$(shell pkg-config alsa jack --cflags)
LDFLAGS=$(shell pkg-config alsa jack --libs)
ifeq ($(DEBUG),true)
CFLAGS+=-Wall -g
endif

programs=vampisynth
vampisynth_SRCS=synth.c envelope.c filter.c oscillator.c voice.c main.c

include common.mk/common.mk
