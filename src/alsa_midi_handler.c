#include <stdio.h>
#include "alsa_midi_handler.h"
#include "cli_utils.h"
#include <alsa/asoundlib.h>

#define PROMPT_MIDI_SENDERS_MAX_PORTS 32 // Max amount of ports that are displayed by prompt midi sender function

// Ask user which MIDI controller (senders) to use
// Sender clients are found under ports in a hiearchy
// WARNING: IT IGNORES MANY VIRTUAL AND SYSTEM CLIENTS
MidiAddress prompt_midi_senders(snd_seq_t *seq) {
	// Struct to store midi addresses that will show up in each entry
	char *names[PROMPT_MIDI_SENDERS_MAX_PORTS];
	MidiAddress *addresses[PROMPT_MIDI_SENDERS_MAX_PORTS];
	int idx = 0; // Keep track of indexs of each entry

	// These are for storing info about clients and ports
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;
	// Allocate memory for them (no need to free)
	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);

	printf("Select a midi input from the options below. (j/k/enter)\n");

	// Set current client to -1 (nothing)
	snd_seq_client_info_set_client(cinfo, -1);
	// This function goes to the next available client
	// And outputs <0 when it reaches the end, breaking the while loop.
	while(snd_seq_query_next_client(seq, cinfo) >= 0) {
		int client_num = snd_seq_client_info_get_client(cinfo);
		
		// Similarly, Iterate through the ports under this client
		snd_seq_port_info_set_port(pinfo, -1);
		while(snd_seq_query_next_port(seq, pinfo) >= 0) {
			unsigned int capabilities = snd_seq_port_info_get_capability(pinfo);
			// Check if the capabilities READ bit is 1 using bitwise AND(&) the constants (e.g. SND_SEQ_PORT_CAP_READ) which tells which bit it is
			// Add 'type' to your check
			if ((capabilities & SND_SEQ_PORT_CAP_READ) && 
				(capabilities & SND_SEQ_PORT_CAP_SUBS_READ) &&
				(capabilities & (SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_HARDWARE))) {

				addresses[idx] = malloc(sizeof(MidiAddress));

				addresses[idx]->port = snd_seq_port_info_get_port(pinfo);
				addresses[idx]->client = client_num;

				asprintf(&names[idx], "[%d][%d] %s (%s)",
					client_num,
					snd_seq_port_info_get_port(pinfo),
					snd_seq_client_info_get_name(cinfo),
					snd_seq_port_info_get_name(pinfo));

				idx++;
			}
		}
	}
	names[idx] = NULL; // Mark end of list

	int chosenidx = cli_menu(names);
	printf("Chosen address: [%d:%d]\n", addresses[chosenidx]->client, addresses[chosenidx]->port);

	MidiAddress result = *addresses[chosenidx];

	// Free each name and address used.
	for (int i = 0; i < idx; i++) {
		free(names[i]);
		free(addresses[i]);
	}
	return result;
}

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

	// + CREATE CLIENT PORT THAT THE MIDI CONTROLLER WILL CONNECT TO +
	int port_create_err = snd_seq_create_simple_port(seq, "Input", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION); 
	if(port_create_err < 0) {
		printf("Whoopse, could not create port: %s\n", snd_strerror(port_create_err));
		return NULL;
	}
	// + CONNECT/SUBCRIBE MIDI CONTROLLER PORT TO CLIENT PORT	
	snd_seq_port_subscribe_t *subs; // Create a subription struct to store settings
	// allocate memory (stack) for the struct
	snd_seq_port_subscribe_alloca(&subs);
	// Midi controller port address
	MidiAddress sender_address = prompt_midi_senders(seq);
	snd_seq_addr_t sender = { .client = sender_address.client, .port = sender_address.port };
	// Destination port address (the port we want the midi controller to subscribe to)
	snd_seq_addr_t dest = { .client = snd_seq_client_id(seq), .port = 0 };
	// Define subscription sender(midi controller) and destination(our port from earlier)
	snd_seq_port_subscribe_set_sender(subs, &sender);
	snd_seq_port_subscribe_set_dest(subs, &dest);
	// Finally, pass on the struct to alsa
	int seq_sub_error = snd_seq_subscribe_port(seq, subs); 
	if(seq_sub_error < 0) {
		printf("Whoopse, couldn't subcribe your midi controller with client port: %s\n", snd_strerror(seq_sub_error));
		return NULL;
	}
	printf("Connected to your midi input!\n");
	return seq;
}
