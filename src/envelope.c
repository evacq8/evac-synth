#include <envelope.h>

/* This function updates envelope levels and also changes the following envelopes states when appropriate:
 * Attack -> Decay
 * Decay -> Sustain
 * Release -> Note Free
 *
 * Any -> Release | is triggered in synthesis.c (note_off function)
 * Note Free -> Attack | is triggered in synthesis.c (note_on function)
 *
 * env_level is reset (0.0) on note_on function in synthesis.c
 * */

// All times are in seconds.
float attack_time = 0.005;
float decay_time = 0.5;
float sustain_level = 0.7f;
float release_time = 0.8;
void envelope_tick(float *env_level, EnvelopeState *env_state) {
	if(*env_state == ENVELOPE_SUSTAIN) return; // No changes needed if its at sustain (sustain -> release transition handled in note_off func)
	float delta_time = 1.0f/44100; // seconds
	// increment levels based on state
	float step;
	if(*env_state == ENVELOPE_ATTACK) {
		step = delta_time/attack_time;
		// If this step will make env_level reach 1, transition from attack -> decay
		if(*env_level+step > 1) {
			*env_level = 1;
			*env_state = ENVELOPE_DECAY;
			return;
		}
	} else if (*env_state == ENVELOPE_DECAY) {
		step = -delta_time/decay_time;
		// If this step will make env_level reach sustain_level, transition from decay -> sustain
		if(*env_level+step < sustain_level) {
			*env_level = sustain_level;
			*env_state = ENVELOPE_SUSTAIN;
			return;
		}
	} else if (*env_state == ENVELOPE_RELEASE) {
		step = -delta_time/release_time;
		// If this step will make env_level reach 0, transition from release -> free
		if(*env_level+step < 0) {
			*env_level = 0;
			*env_state = ENVELOPE_FREE;
			return;
		}
	} else step = 0; // to prevent warning complaining about how step could be used uninitialized.
	*env_level += step;
}
