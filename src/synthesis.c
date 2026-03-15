#include "alsa_audio_handler.h"
#include "alsa_midi_handler.h"
#include "oscillator.h"
#include "envelope.h"
#include "filter.h"

#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define MAX_NOTES 9 // how many elements are in the notes[] array? this will control the max amt of notes which can be pressed at a time

typedef struct {
	int note; // Range: 0-127, where 127 is G9 and 0 is C-1
	float velocity; // How hard the note was pressed on the midi controller. (127 max)
	float phase; // Current NORMALIZED (0.0 - 1.0) oscillator phase of this note
	float vibrato_phase; // extra LFO vibrato phase
	float increment; // How much to add to phase for this note?

	EnvelopeState env_state; // Current envelope state of the note (see envelope.h for all)
	float env_level; // Loudness of the note's envelope (0.0 - 1.0)
} Note;

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
	for (int i = 0; i < MAX_NOTES; i++) {
		// Find first note which is free and take its place
		if(notes[i].env_state == ENVELOPE_FREE) {
			notes[i].note = notenum;
			notes[i].velocity = velocity;
			// randomize phase to stop massive contructive interference when many notes are played at once
			notes[i].phase = ((float)rand()/RAND_MAX);
			float freq = 440*pow(2, (float)(notenum - 69)/12); // Convert notenum to frequency
			notes[i].increment = freq/44100; // Convert frequency to increment
			notes[i].env_level = 0;
			notes[i].env_state = ENVELOPE_ATTACK; // Set note to attack
			break; // do NOT search anymore
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
	biquad_setup_bandpass(850, 20, &formant1);
	BiquadVariables formant2;
	biquad_setup_bandpass(1220, 25, &formant2);
	BiquadVariables formant3;
	biquad_setup_bandpass(2800, 25, &formant3);

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
				
				// sample = oscillator * 10,000 * velocity * env_level
				int16_t current_sample = get_wave_table_sample(notes[n].phase,fundamental_osc_table)*10000*(notes[n].velocity/127)*notes[n].env_level;

				// aspiration
				current_sample += (0.0015*32768*notes[n].env_level) * (((float)rand() / (float)RAND_MAX)*2 - 1);


				// Increment phase
				float vibrato_wobble = get_wave_table_sample(notes[n].vibrato_phase,vibrato_osc_table);
				while (notes[n].vibrato_phase >= 1) notes[n].vibrato_phase -= 1;
				notes[n].vibrato_phase += 5.5f / 44100.0f; // increment vibrato oscillator
				notes[n].phase+=notes[n].increment * (1.0f + (vibrato_wobble * 0.005f * notes[n].env_level)); // increment fundimental phase


				while (notes[n].phase >= 1) notes[n].phase -= 1; // reset phase when making full oscilation
				envelope_tick(&notes[n].env_level, &notes[n].env_state); // Update envelope states and level

				mixed_sample+=current_sample; // add this notes stuff to the mixed sample
			}

			int16_t f1_output = biquad_filter(mixed_sample, &formant1);
			int16_t f2_output = biquad_filter(mixed_sample, &formant2);
			int16_t f3_output = biquad_filter(mixed_sample, &formant3);

			mixed_sample = (f1_output)+(f2_output/2)+(f3_output/4);

			// write to both channels
			buffer[2*i] = mixed_sample;
			buffer[2*i+1] = mixed_sample;
		}
		write_to_alsa_buffer(pcm, buffer);
	}

	return 0;
}
