#include <stdio.h>
#include "alsa_midi_handler.h"
#include <alsa/asoundlib.h>

snd_seq_t* initialize_seq() {
	snd_seq_t *seq; // Handle to the sequencer (midi intrument)
	/* This line opens the sequencer
	 * 1st argument - assign handle to 'seq'
	 * 2nd argument - "default" selects system default sequencer
	 * 3rd argument - specifies we only want input and not output/duplex(both)
	 * 4th argument - 0 means blocking mode; halt program while calling access functions
	 * return - returns negative values upon errors
	 */
	int seq_open_err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0);
	if(seq_open_err < 0) {
		printf("Whoopse, could not open sequencer: %s\n", snd_strerror(seq_open_err));
		return NULL;
	}
	printf("Opened Sequencer.\n");
	// + CREATE CLIENT PORT THAT THE MIDI CONTROLLER WILL CONNECT TO +
	int port_create_err = snd_seq_create_simple_port(seq, "Input", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION); 
	if(port_create_err < 0) {
		printf("Whoopse, could not create port: %s\n", snd_strerror(port_create_err));
		return NULL;
	}
	printf("Created client port!\n");
	// + CONNECT/SUBCRIBE MIDI CONTROLLER PORT TO CLIENT PORT	
	snd_seq_port_subscribe_t *subs; // Create a subription struct to store settings
	// allocate memory (stack) for the struct
	snd_seq_port_subscribe_alloca(&subs);
	// Midi controller port address
	snd_seq_addr_t sender = { .client = 20, .port = 0 };
	// Client port address
	snd_seq_addr_t dest = { .client = snd_seq_client_id(seq), .port = 0 };
	// Define subscription sender(midi controller) and destination(our port from earlier)
	snd_seq_port_subscribe_set_sender(subs, &sender);
	snd_seq_port_subscribe_set_dest(subs, &dest);
	// Finally, pass on the struct to alsa
	int seq_sub_error = snd_seq_subscribe_port(seq, subs); 
	if(seq_sub_error < 0) {
		printf("Whoopse, could'nt subcribe your midi controller with client port: %s\n", snd_strerror(seq_sub_error));
		return NULL;
	}
	printf("Connected your midi controller to client port\n");
	return seq;
}
