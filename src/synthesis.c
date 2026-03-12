#include "alsa_audio_handler.h"
#include "alsa_midi_handler.h"
#include "oscillator.h"
#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define MAX_NOTES 9 // how many elements are in the notes[] array? this will control the max amt of notes which can be pressed at a time

typedef struct {
	int note; // Range: 0-127, where 127 is G9 and 0 is C-1
	float velocity; // How hard the note was pressed on the midi controller. (127 max)
	float phase; // Current NORMALIZED (0.0 - 1.0) oscillator phase of this note
	float increment; // How much to add to phase for this note?
	int active; // is note being pressed?
} Note;

void note_on(Note notes[], int notenum, int velocity) {
	for (int i = 0; i < MAX_NOTES; i++) {
		// Find first note which is not active (to fill its place)
		if(notes[i].active != 1) {
			notes[i].note = notenum;
			notes[i].velocity = velocity;
			// randomize phase to stop massive contructive interference when many notes are played at once
			notes[i].phase = ((float)rand()/RAND_MAX);
			float freq = 440*pow(2, (float)(notenum - 69)/12); // Convert notenum to frequency
			notes[i].increment = freq/44100; // Convert frequency to increment
			notes[i].active = 1;
			break; // do NOT search anymore
		}
	}
}

void note_off(Note notes[], int notenum) {
	for(int i = 0; i < MAX_NOTES; i++) {
		if(notes[i].note == notenum) {
			notes[i].active = 0;
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
	Note notes[MAX_NOTES] = {0}; // 16 max notes
	
	// Set oscillator:
	set_wave_table_oscillator(OSCILLATOR_TRIANGLE);
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
				if (notes[n].active != 1) continue; // IGNORE UNACTIVE NOTES!
				notes[n].phase+=notes[n].increment; // add increment to phase
				while (notes[n].phase >= 1) notes[n].phase -= 1; // reset phase when making full oscilation

				
				mixed_sample+=get_wave_table_sample(notes[n].phase)*10000*(notes[n].velocity/127); // add this notes stuff to the mixed sample
			}

			// Soft clipping
			float norm = mixed_sample / 32767.0f;
			norm = tanhf(norm);
			mixed_sample = (int16_t)(norm*32767);

			// write to both channels
			buffer[2*i] = mixed_sample;
			buffer[2*i+1] = mixed_sample;
		}
		write_to_alsa_buffer(pcm, buffer);
	}

	return 0;
}
