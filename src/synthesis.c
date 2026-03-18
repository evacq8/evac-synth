#include "alsa_audio_handler.h"
#include "alsa_midi_handler.h"
#include "oscillator.h"
#include "envelope.h"
#include "filter.h"

#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define MAX_NOTES 9 // how many elements are in the notes[] array? this will control the max amt of notes which can be pressed at a time

int amount_oscillators = 2;

// used to identify which oscillator this is. (e.g. Fundimental, Choir_voice, LFO_Vibrato, etc.
typedef enum {
	OSC_ID_FUNDAMENTAL, // If this synth uses only one main oscillator, this is the fundamental.
	OSC_ID_LFO_VIBRATO, // A Low Frequency Oscillator which changes changes frequency
} OscillatorIdentifier;

// Holds data needed for handling a single oscillator
typedef struct {
	float phase;
	float increment;
	OscillatorIdentifier id;
} OscillatorUnit;

typedef struct {
	int note; // Range: 0-127, where 127 is G9 and 0 is C-1
	float velocity; // How hard the note was pressed on the midi controller? (127 max)
	OscillatorUnit oscillators[2];
	EnvelopeState env_state; // Current envelope state of the note (see envelope.h for all)
	float env_level; // Loudness of the note's envelope (0.0 - 1.0)
} Note;

void setup_note_oscillators(Note notes[]) {
	for (int n = 0; n < MAX_NOTES; n++) {
		for (int o = 0; o < amount_oscillators; o++) {
			if(o == 0) notes[n].oscillators[o].id = OSC_ID_LFO_VIBRATO;
			else if(o == 1) {
				notes[n].oscillators[o].id = OSC_ID_FUNDAMENTAL;
				notes[n].oscillators[o].increment = 4.4f/44100;
			}
		}
	}
}

void note_on(Note notes[], int notenum, int velocity) {

	// First check if the requested notenum already actively exists (fading out due to envelope release)
	for (int i = 0; i < MAX_NOTES; i++) {
		if(notes[i].note == notenum && notes[i].env_state != ENVELOPE_FREE) { // if so, reset velocity and set state to attack
			notes[i].velocity = velocity;
			notes[i].env_state = ENVELOPE_ATTACK;
			return;
		}
	}

	// Otherwise, find the first inactive note
	for (int n = 0; n < MAX_NOTES; n++) {
		// Find first note which is free and take its place
		if(notes[n].env_state == ENVELOPE_FREE) {
			notes[n].note = notenum;
			notes[n].velocity = velocity;
			// Set up oscillators
			for (int o = 0; o < amount_oscillators; o++) {
				if(notes[n].oscillators[o].id == OSC_ID_FUNDAMENTAL) {
					float freq = 440*pow(2, (float)(notenum - 69)/12); // Convert notenum to frequency
					notes[n].oscillators[o].increment = freq/44100;
				} else if (notes[n].oscillators[o].id == OSC_ID_LFO_VIBRATO) {
					notes[n].oscillators[o].increment = 5.5f/44100;
				}
				// Randomize phase to stop massive contructive interference when many oscillators start at once.
				notes[n].oscillators[o].phase = ((float)rand()/RAND_MAX);
			}
			notes[n].env_level = 0;
			notes[n].env_state = ENVELOPE_ATTACK; // Set note to attack
			return; // do NOT search anymore
		}
	}
}

void note_off(Note notes[], int notenum) {
	for(int i = 0; i < MAX_NOTES; i++) {
		if(notes[i].note == notenum) {
			// Set note to release it the note is no longer being pressed
			notes[i].env_state = ENVELOPE_RELEASE;
			break;
		}
	}
}

int main() {
	snd_pcm_t *pcm = initialize_pcm("default");
	if(!pcm) return 1;
	snd_seq_t *seq = initialize_seq();
	if(!seq) return 1;
	/* Buffer to where to write audio
	 *	Needs to hold 4096 frames
	 *	Each frame has 2 channels and 2 bytes per channel
	 *	thus, we need the buffer to hold 4096*2*2 = 16,384 bytes
	 *	int16_t holds 2 bytes, so we need a buffer of 16,384/2 = 8192 int16_t's
	 *	Note: int16_t has range of -32,768 to +32,767
	 */
	int16_t buffer[8192];
	Note notes[MAX_NOTES] = {0};
	
	// Create oscillator look up tables
	int16_t fundamental_osc_table[OSCILLATOR_TABLE_SIZE]; // oscillator which creates base sound
	set_wave_table_oscillator(OSCILLATOR_GLOTTAL, fundamental_osc_table);
	int16_t vibrato_osc_table[OSCILLATOR_TABLE_SIZE]; // LFO which controls pitch variations
	set_wave_table_oscillator(OSCILLATOR_SINE, vibrato_osc_table);
	
	BiquadVariables formant1;
	biquad_setup_bandpass(850, 8, &formant1);
	BiquadVariables formant2;
	biquad_setup_bandpass(1220, 8, &formant2);
	BiquadVariables formant3;
	biquad_setup_bandpass(2800, 8, &formant3);

	while(1) {
		// + Convert midi events to note elements to add to notes[] +
		
		// + Synthesis +
		for(int i = 0; i < 4096; i++) {
			snd_seq_event_t *ev;
			// Read all pending sequencer events
			while(snd_seq_event_input_pending(seq, 1)>0) {
				snd_seq_event_input(seq, &ev);
				if(ev->type == SND_SEQ_EVENT_NOTEON) {
					note_on(notes, ev->data.note.note, ev->data.note.velocity);
				} else if(ev->type == SND_SEQ_EVENT_NOTEOFF) {
					note_off(notes, ev->data.note.note);
				}
				snd_seq_free_event(ev);
			}

			// AUDIO LOOP
			int16_t mixed_sample=0; // A mix of all samples created by each note
			for(int n = 0; n < MAX_NOTES; n++) {
				if (notes[n].env_state == ENVELOPE_FREE) continue; // IGNORE UNACTIVE (i.e. free) NOTES!
				
				int16_t note_sample = 0;

				float vibrato_wobble = 0;
				for(int o = 0; o < amount_oscillators; o++) {

					if (notes[n].oscillators[o].id == OSC_ID_LFO_VIBRATO) {
						vibrato_wobble = get_wave_table_sample(notes[n].oscillators[o].phase,vibrato_osc_table);
						notes[n].oscillators[o].phase += notes[n].oscillators[o].increment;
					} else if (notes[n].oscillators[o].id == OSC_ID_FUNDAMENTAL) {
						// Create this note's sample
						float osc_output = get_wave_table_sample(notes[n].oscillators[o].phase,fundamental_osc_table);
						note_sample = osc_output * 10000 * (notes[n].velocity/127) * notes[n].env_level;

						notes[n].oscillators[o].phase += notes[n].oscillators[o].increment * (1.0f + (vibrato_wobble * 0.006f * notes[n].env_level)); // Add additional wobble to fundimental phase
					}				
					// Kick back phase once it crosses 1.0 (keep it normalized)
					while(notes[n].oscillators[o].phase >= 1) notes[n].oscillators[o].phase -= 1;
				}

				// per note aspiration white noise layer
				note_sample += (0.0015*32768*notes[n].env_level) * (((float)rand() / (float)RAND_MAX)*2 - 1);

				// Update the envelope level and state (if necessary) for this note.
				envelope_tick(&notes[n].env_level, &notes[n].env_state); // Update envelope states and level

				mixed_sample+=note_sample; // Add this notes stuff to the mixed sample
			}

			int16_t f1_output = biquad_filter(mixed_sample, &formant1);
			int16_t f2_output = biquad_filter(mixed_sample, &formant2);
			int16_t f3_output = biquad_filter(mixed_sample, &formant3);

			mixed_sample = (f1_output*2)+(f2_output)+(f3_output/1.5);

			// write to both channels
			buffer[2*i] = mixed_sample;
			buffer[2*i+1] = mixed_sample;
		}
		write_to_alsa_buffer(pcm, buffer);
	}

	return 0;
}
