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
#include <jack/transport.h>

#include <asoundlib.h>

#include "synth.h"
struct Synth synth;

typedef jack_default_audio_sample_t sample_t;

const double PI = 3.14;

jack_client_t *client;
jack_port_t *output_port;
unsigned long sr;
int freq = 880;
int bpm;
jack_nframes_t tone_length, wave_length;
sample_t *wave;
long offset = 0;
int transport_aware = 0;
jack_transport_state_t transport_state;

void usage () {
	fprintf (stderr, "\n"
"usage: jack_metro \n"
"              [ --frequency OR -f frequency (in Hz) ]\n"
"              [ --amplitude OR -A maximum amplitude (between 0 and 1) ]\n"
"              [ --duration OR -D duration (in ms) ]\n"
"              [ --attack OR -a attack (in percent of duration) ]\n"
"              [ --decay OR -d decay (in percent of duration) ]\n"
"              [ --name OR -n jack name for metronome client ]\n"
"              [ --transport OR -t transport aware ]\n"
"              --bpm OR -b beats per minute\n"
);
}

void
process_silence (jack_nframes_t nframes)
{
	sample_t *buffer = (sample_t *) jack_port_get_buffer (output_port, nframes);
	memset (buffer, 0, sizeof (jack_default_audio_sample_t) * nframes);
}

void
process_audio (jack_nframes_t nframes)
{

	sample_t *buffer = (sample_t *) jack_port_get_buffer (output_port, nframes);
	for(int i = 0; i < nframes; i++) {
		buffer[i] = synth_render_sample(&synth) / 32767.0;
	}
	// jack_nframes_t frames_left = nframes;

	// while (wave_length - offset < frames_left) {
	// 	memcpy (buffer + (nframes - frames_left), wave + offset, sizeof (sample_t) * (wave_length - offset));
	// 	frames_left -= wave_length - offset;
	// 	offset = 0;
	// }
	// if (frames_left > 0) {
	// 	memcpy (buffer + (nframes - frames_left), wave + offset, sizeof (sample_t) * frames_left);
	// 	offset += frames_left;
	// }
}

int
process (jack_nframes_t nframes, void *arg)
{
	if (transport_aware) {
		jack_position_t pos;

		if (jack_transport_query (client, &pos)
				!= JackTransportRolling) {

			process_silence (nframes);
			return 0;
		}
		offset = pos.frame % wave_length;
	}
	process_audio (nframes);
	return 0;
}

int
sample_rate_change () {
	printf("Sample rate has changed! Exiting...\n");
	exit(-1);
}

static char *midi_note_names[12] = {
	"C",
	"C#",
	"D",
	"D#",
	"E",
	"F",
	"F#",
	"G",
	"G#",
	"A",
	"A#",
	"B",
};

const char *midi_note_name(int note) {
	return midi_note_names[note % 12];
}

snd_seq_t *open_seq();
void midi_action(snd_seq_t *seq_handle);

snd_seq_t *open_seq() {

	snd_seq_t *seq_handle;
	int portid;

	if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		exit(1);
	}
	snd_seq_set_client_name(seq_handle, "Vampi Synth");
	if ((portid = snd_seq_create_simple_port(seq_handle, "Cornhole",
						SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
						SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		exit(1);
	}
	return(seq_handle);
}

void midi_action(snd_seq_t *seq_handle) {

	snd_seq_event_t *ev;

	do {
		snd_seq_event_input(seq_handle, &ev);
		switch (ev->type) {
		case SND_SEQ_EVENT_NOTEON:
			printf("Note on %d (%s) %d\n", ev->data.note.note, midi_note_name(ev->data.note.note), ev->data.note.velocity);
			synth_note_on(&synth, ev->data.note.note, ev->data.note.velocity);
			break;
		case SND_SEQ_EVENT_NOTEOFF:
			printf("Note off %d\n", ev->data.note.note);
			synth_note_off(&synth, ev->data.note.note, ev->data.note.velocity);
			break;
		}
		snd_seq_free_event(ev);
	} while (snd_seq_event_input_pending(seq_handle, 0) > 0);
}


int main(int argc, char **argv) {
	sample_t scale;
	int i, attack_length, decay_length;
	double *amp;
	double max_amp = 0.5;
	int option_index;
	int opt;
	int got_bpm = 0;
	int attack_percent = 1, decay_percent = 10, dur_arg = 100;
	char *client_name = 0;
	char *bpm_string = "bpm";
	int verbose = 0;
	jack_status_t status;

	const char *options = "f:A:D:a:d:b:n:thv";
	struct option long_options[] =
	{
		{"name", 1, 0, 'n'},
		{"transport", 0, 0, 't'},
		{"help", 0, 0, 'h'},
		{"verbose", 0, 0, 'v'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {
		switch (opt) {
		case 'n':
			client_name = (char *) malloc (strlen (optarg) * sizeof (char));
			strcpy (client_name, optarg);
			break;
		case 't':
			transport_aware = 1;
			break;
		default:
			fprintf (stderr, "unknown option %c\n", opt);
		case 'h':
			usage ();
			return -1;
		case 'v':
			verbose = 1;
			break;
		}
	}

	/* Initial Jack setup, get sample rate */
	if (!client_name) {
		client_name = (char *) malloc (9 * sizeof (char));
		strcpy (client_name, "metro");
	}
	if ((client = jack_client_open (client_name, JackNoStartServer, &status)) == 0) {
		fprintf (stderr, "jack server not running?\n");
		return 1;
	}
	jack_set_process_callback (client, process, 0);
	output_port = jack_port_register (client, bpm_string, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	sr = jack_get_sample_rate (client);

	synth_init(&synth);
	synth.attack = 50;
	synth.decay = 200;
	synth.sustain = 60;
	synth.release = 100;

	/* setup wave table parameters */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		return 1;
	}

	snd_seq_t *seq_handle;
	int npfd;
	struct pollfd *pfd;

	seq_handle = open_seq();
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
	while (1) {
		printf("poll\n");
		if (poll(pfd, npfd, 100000) > 0) {
			midi_action(seq_handle);
		}
	}
}
