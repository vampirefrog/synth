PKGS=alsa jack gtk+-3.0
CFLAGS=$(shell pkg-config $(PKGS) --cflags)
LDFLAGS=$(shell pkg-config $(PKGS) --libs)
ifeq ($(DEBUG),true)
CFLAGS+=-Wall -g
endif

programs=vampisynth
vampisynth_SRCS=synth.c envelope.c filter.c oscillator.c voice.c sineosc.c main.c

include common.mk/common.mk
