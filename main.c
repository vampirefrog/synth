/*
		Copyright (C) 2002 Anthony Van Groningen

		This program is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; either version 2 of the License, or
		(at your option) any later version.
		This program is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
		along with this program; if not, write to the Free Software
		Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>
#include <string.h>

#include <jack/jack.h>

#include <alsa/asoundlib.h>

#include "synth.h"
#include "midi.h"

struct Synth synth;

typedef jack_default_audio_sample_t sample_t;

#define CLIENT_NAME "Vampi Synth"
jack_client_t *client;
jack_port_t *output_ports[2];
unsigned long sr;
int verbose = 0;

int process(jack_nframes_t nframes, void *arg) {
	sample_t *buffers[2];
	buffers[0] = (sample_t *) jack_port_get_buffer(output_ports[0], nframes);
	buffers[1] = (sample_t *) jack_port_get_buffer(output_ports[1], nframes);
	for(int i = 0; i < nframes; i++) {
		float out[2] = {0, 0};
		synth_render_sample(&synth, out);
		buffers[0][i] = out[0];
		buffers[1][i] = out[1];
	}
	return 0;
}

static char *midi_note_names[12] = {
	"C", "C#", "D", "D#", "E", "F",
	"F#", "G", "G#", "A", "A#", "B",
};

const char *midi_note_name(int note) {
	return midi_note_names[note % 12];
}

const char *midi_cc_name(int cc) {
	switch(cc) {
		case MIDI_CTL_ALL_NOTES_OFF: return "All notes off";
		case MIDI_CTL_ALL_SOUNDS_OFF: return "All sounds off";
		case MIDI_CTL_DATA_DECREMENT: return "Data Decrement";
		case MIDI_CTL_DATA_INCREMENT: return "Data Increment";
		case MIDI_CTL_E1_REVERB_DEPTH: return "E1 Reverb Depth";
		case MIDI_CTL_E2_TREMOLO_DEPTH: return "E2 Tremolo Depth";
		case MIDI_CTL_E3_CHORUS_DEPTH: return "E3 Chorus Depth";
		case MIDI_CTL_E4_DETUNE_DEPTH: return "E4 Detune Depth";
		case MIDI_CTL_E5_PHASER_DEPTH: return "E5 Phaser Depth";
		case MIDI_CTL_GENERAL_PURPOSE5: return "General purpose 5";
		case MIDI_CTL_GENERAL_PURPOSE6: return "General purpose 6";
		case MIDI_CTL_GENERAL_PURPOSE7: return "General purpose 7";
		case MIDI_CTL_GENERAL_PURPOSE8: return "General purpose 8";
		case MIDI_CTL_HOLD2: return "Hold2";
		case MIDI_CTL_LEGATO_FOOTSWITCH: return "Legato foot switch";
		case MIDI_CTL_LOCAL_CONTROL_SWITCH: return "Local control switch";
		case MIDI_CTL_LSB_BALANCE: return "Balance";
		case MIDI_CTL_LSB_BANK: return "Bank selection";
		case MIDI_CTL_LSB_BREATH: return "Breath";
		case MIDI_CTL_LSB_DATA_ENTRY: return "Data entry";
		case MIDI_CTL_LSB_EFFECT1: return "Effect1";
		case MIDI_CTL_LSB_EFFECT2: return "Effect2";
		case MIDI_CTL_LSB_EXPRESSION: return "Expression";
		case MIDI_CTL_LSB_FOOT: return "Foot";
		case MIDI_CTL_LSB_GENERAL_PURPOSE1: return "General purpose 1";
		case MIDI_CTL_LSB_GENERAL_PURPOSE2: return "General purpose 2";
		case MIDI_CTL_LSB_GENERAL_PURPOSE3: return "General purpose 3";
		case MIDI_CTL_LSB_GENERAL_PURPOSE4: return "General purpose 4";
		case MIDI_CTL_LSB_MAIN_VOLUME: return "Main volume";
		case MIDI_CTL_LSB_MODWHEEL: return "Modulation";
		case MIDI_CTL_LSB_PAN: return "Panpot";
		case MIDI_CTL_LSB_PORTAMENTO_TIME: return "Portamento time";
		case MIDI_CTL_MONO1: return "Mono1";
		case MIDI_CTL_MONO2: return "Mono2";
		case MIDI_CTL_MSB_BALANCE: return "Balance";
		case MIDI_CTL_MSB_BANK: return "Bank selection";
		case MIDI_CTL_MSB_BREATH: return "Breath";
		case MIDI_CTL_MSB_DATA_ENTRY: return "Data entry";
		case MIDI_CTL_MSB_EFFECT1: return "Effect1";
		case MIDI_CTL_MSB_EFFECT2: return "Effect2";
		case MIDI_CTL_MSB_EXPRESSION: return "Expression";
		case MIDI_CTL_MSB_FOOT: return "Foot";
		case MIDI_CTL_MSB_GENERAL_PURPOSE1: return "General purpose 1";
		case MIDI_CTL_MSB_GENERAL_PURPOSE2: return "General purpose 2";
		case MIDI_CTL_MSB_GENERAL_PURPOSE3: return "General purpose 3";
		case MIDI_CTL_MSB_GENERAL_PURPOSE4: return "General purpose 4";
		case MIDI_CTL_MSB_MAIN_VOLUME: return "Main volume";
		case MIDI_CTL_MSB_MODWHEEL: return "Modulation";
		case MIDI_CTL_MSB_PAN: return "Panpot";
		case MIDI_CTL_MSB_PORTAMENTO_TIME: return "Portamento time";
		case MIDI_CTL_NONREG_PARM_NUM_LSB: return "Non-registered parameter number";
		case MIDI_CTL_NONREG_PARM_NUM_MSB: return "Non-registered parameter number";
		case MIDI_CTL_OMNI_OFF: return "Omni off";
		case MIDI_CTL_OMNI_ON: return "Omni on";
		case MIDI_CTL_PORTAMENTO: return "Portamento";
		case MIDI_CTL_PORTAMENTO_CONTROL: return "Portamento control";
		case MIDI_CTL_REGIST_PARM_NUM_LSB: return "Registered parameter number";
		case MIDI_CTL_REGIST_PARM_NUM_MSB: return "Registered parameter number";
		case MIDI_CTL_RESET_CONTROLLERS: return "Reset Controllers";
		case MIDI_CTL_SC10: return "SC10";
		case MIDI_CTL_SC1_SOUND_VARIATION: return "SC1 Sound Variation";
		case MIDI_CTL_SC2_TIMBRE: return "SC2 Timbre";
		case MIDI_CTL_SC3_RELEASE_TIME: return "SC3 Release Time";
		case MIDI_CTL_SC4_ATTACK_TIME: return "SC4 Attack Time";
		case MIDI_CTL_SC5_BRIGHTNESS: return "SC5 Brightness";
		case MIDI_CTL_SC6: return "SC6";
		case MIDI_CTL_SC7: return "SC7";
		case MIDI_CTL_SC8: return "SC8";
		case MIDI_CTL_SC9: return "SC9";
		case MIDI_CTL_SOFT_PEDAL: return "Soft pedal";
		case MIDI_CTL_SOSTENUTO: return "Sostenuto";
		case MIDI_CTL_SUSTAIN: return "Sustain pedal";
	}

	return "Unknown";
}

void midi_action(snd_seq_t *seq_handle) {
	snd_seq_event_t *ev;

	do {
		snd_seq_event_input(seq_handle, &ev);
		switch (ev->type) {
			case SND_SEQ_EVENT_NOTEON:
				if(verbose)
					printf("Note on %s (%d) %d\n", midi_note_name(ev->data.note.note), ev->data.note.note, ev->data.note.velocity);
				synth_midi_note_on(&synth, ev->data.note.note, ev->data.note.velocity);
				break;
			case SND_SEQ_EVENT_NOTEOFF:
				if(verbose)
					printf("Note off %s (%d) %d\n", midi_note_name(ev->data.note.note), ev->data.note.note, ev->data.note.velocity);
				synth_midi_note_off(&synth, ev->data.note.note, ev->data.note.velocity);
				break;
			case SND_SEQ_EVENT_PITCHBEND:
				synth_midi_pitch_bend(&synth, ev->data.control.value);
				break;
			case SND_SEQ_EVENT_CONTROLLER:
				if(verbose)
					printf("CC 0x%02x (%s) %d\n", ev->data.control.param, midi_cc_name(ev->data.control.param), ev->data.control.value);
				synth_midi_cc(&synth, ev->data.control.param, ev->data.control.value);
				break;
		}
		snd_seq_free_event(ev);
	} while (snd_seq_event_input_pending(seq_handle, 0) > 0);
}

int main(int argc, char **argv) {
	int option_index;
	int opt;
	char *client_name = CLIENT_NAME;
	char *input_seq_addr = 0;
	jack_status_t status;

	const char *options = "n:p:hv";
	struct option long_options[] = {
		{"name", 1, 0, 'n'},
		{"port", 1, 0, 'p'},
		{"help", 0, 0, 'h'},
		{"verbose", 0, 0, 'v'},
		{0, 0, 0, 0}
	};

	while((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {
		switch(opt) {
			case 'n':
				client_name = (char *) malloc(strlen (optarg) * sizeof (char));
				strcpy (client_name, optarg);
				break;
			case 'p':
				input_seq_addr = optarg;
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				fprintf (stderr, "Unknown option %c\n", opt);
			case 'h':
				fprintf(
					stderr,
					"usage: %s [options]\n"
					"\t-p, --port <port name>   Input port for sequencer events (MIDI keyboard)\n"
					"\t-n, --name <name>.       Jack client name. Default: " CLIENT_NAME " ]\n"
					"\t-v    Verbose\n"
					"\t-h    Help\n",
					argv[0]
				);
				return -1;
		}
	}

	client = jack_client_open(client_name, JackNullOption, &status, NULL);
	if(client == NULL) {
		fprintf (stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
		if(status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		return 1;
	}
	if(status & JackServerStarted) {
		fprintf(stderr, "JACK server started\n");
	}
	if(status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf(stderr, "unique name `%s' assigned\n", client_name);
	}

	jack_set_process_callback(client, process, 0);
	output_ports[0] = jack_port_register(client, "Synth Out L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	if(!output_ports[0]) {
		fprintf(stderr, "Could not register Synth Out L port\n");
		return 1;
	}
	output_ports[1] = jack_port_register(client, "Synth Out R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	if(!output_ports[1]) {
		fprintf(stderr, "Could not register Synth Out R port\n");
		return 1;
	}

	sr = jack_get_sample_rate(client);
	printf("Sample rate: %d\n", sr);

	synth_init(&synth);

	if(optind < argc) {
		synth_load_patch(&synth, argv[optind]);
	}

	if (jack_activate(client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	const char **ports;
	ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
	if(ports == NULL) {
		fprintf(stderr, "no physical capture ports\n");
		exit (1);
	}

	if(jack_connect(client, jack_port_name(output_ports[0]), ports[0])) {
		fprintf(stderr, "cannot connect input ports\n");
	}

	if(jack_connect(client, jack_port_name(output_ports[1]), ports[1])) {
		fprintf(stderr, "cannot connect input ports\n");
	}

	free(ports);

	snd_seq_t *seq_handle;
	int npfd;
	struct pollfd *pfd;
	int portid;

	if(snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		exit(1);
	}
	snd_seq_set_client_name(seq_handle, client_name);
	if(
		(
			portid = snd_seq_create_simple_port(
				seq_handle,
				"Cornhole",
				SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
				SND_SEQ_PORT_TYPE_APPLICATION
			)
		) < 0
	) {
		fprintf(stderr, "Error creating sequencer port.\n");
		exit(1);
	}

	snd_seq_addr_t seq_input_port;
	if(input_seq_addr) {
		if(snd_seq_parse_address(seq_handle, &seq_input_port, input_seq_addr) == 0) {
			snd_seq_connect_from(seq_handle, portid, seq_input_port.client, seq_input_port.port);
		} else {
			fprintf(stderr, "Could not parse port: %s\n", input_seq_addr);
			exit(1);
		}
	}

	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
	while(1) {
		if (poll(pfd, npfd, 100000) > 0) {
			midi_action(seq_handle);
		}
	}
}
