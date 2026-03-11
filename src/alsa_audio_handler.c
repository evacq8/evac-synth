/* 

(p.s. This code is probably really bad.. I'm sorry. I may be using wrong terminology throughout.)

This file focuses on sending audio to ALSA for linux devices.
To play the audio the script will:
1. Open the PCM sound device (e.g. your laptop's soundcard)
2. Configure hardware parameters
3. Prepare PCM device
4. Send audio buffers to ALSA
*/


#include <stdio.h>
#include <alsa/asoundlib.h> // required for working with alsa
#include "alsa_audio_handler.h"
#include "cli_utils.h"

/* This function handles opening the PCM device, configuring hardware params, and preparing the device.
 * Returns the PCM device handle.
 * Give in a string of the device name (use
 */
snd_pcm_t* initialize_pcm(char device_name[]) {

	// + INITIALIZE PCM DEVICE +
	snd_pcm_t *soundDevice; // Sound device handle
	
	// Prompt user for which device to use...
	

	int err_sound_device = -1; // Did error occur whilst creating handle?
	/* This line assigns handle to soundDevice
	 *	argument 1 - tells to assign the handle to soundDevice
	 *	argument 2 - device name, 'plughw:0,0' is the first device.
	 *		(also uses plugin layer for automatic hardware support)
	 * 	argument 3 - 'SND_PCM_STREAM_PLAYBACK' specifies we want to play audio, not record.
	 * 	argument 4 - '0' enables blocking mode. pauses program while calling write functions
	 * 	return - returns negative values if errors occured
	 */
	err_sound_device = snd_pcm_open(&soundDevice, device_name, SND_PCM_STREAM_PLAYBACK, 0);
	if(err_sound_device < 0) {
		printf("Whoopse, couldn't open PCM device: %s\n", snd_strerror(err_sound_device));
		return NULL;
	} else {
		printf("Yay, Opened device '%s'\n", snd_pcm_name(soundDevice));
	}

	// + SETUP HARDWARE PARAMETERS +
	snd_pcm_hw_params_t *params; // pointer to hardware parameter object
	snd_pcm_hw_params_malloc(&params); // Allocate memory for the object
	snd_pcm_hw_params_any(soundDevice, params);
	// Access Type - Interleaved: First 2 bytes of a frame are right channel, last 2 are left.
	snd_pcm_hw_params_set_access(soundDevice, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	/* Sample Format - Signed 16-bit Little Endian
	 * This means: each sample is 16 bits (2 bytes)
	 * Signed (negative and positive), where 0 means no sound.
	 * Least significant BYTE comes first.
	 */
	snd_pcm_hw_params_set_format(soundDevice, params, SND_PCM_FORMAT_S16_LE);
	// Channels - 2 for Stereo
	snd_pcm_hw_params_set_channels(soundDevice, params, 2);
	/* Sample rate - 44100 Hz
	 * Last argument tells alsa to pick closest sample rate if the selected one isnt available.
	 */
	unsigned int rate = 44100;
	snd_pcm_hw_params_set_rate_near(soundDevice, params, &rate, 0);
	// Buffer size - 4096 frames in the buffer.
	snd_pcm_uframes_t buffer_size = 4096;
	snd_pcm_hw_params_set_buffer_size_near(soundDevice, params, &buffer_size);
	// Period size - 1024 frames are sent to alsa at a time.
	snd_pcm_uframes_t period_size = 1024;
	int dir = 0;
	snd_pcm_hw_params_set_period_size_near(soundDevice, params, &period_size, &dir);
	// Resample if hardware doesn't support? - Yes
	snd_pcm_hw_params_set_rate_resample(soundDevice, params, 1);
	// Finally, apply to PCM device:
	snd_pcm_hw_params(soundDevice, params);
	// Then free up memory
	snd_pcm_hw_params_free(params);
	printf("Hardware parameters have been set. (Hopefully)\n");

	// + PREPARE PCM DEVICE + 
	int err_sound_device_prepare = snd_pcm_prepare(soundDevice); // Returns negative values when error occurs.
	if(err_sound_device_prepare < 0) {
		printf("Whoopse, couldn't prepare PCM device: %s\n", snd_strerror(err_sound_device_prepare)); 
		return NULL;
	} else {
		printf("Yay, PCM Device is now prepared.\n");
	}

	return soundDevice;
}

/* This helper function writes audio to the buffer
 * 1st argument - PCM Device handle
 * 2nd argument - An Interweaved int16_t[8192] buffer
 * Return - 0 = Success, 1 = Fail
 */
int write_to_alsa_buffer(snd_pcm_t *pcm, int16_t *buffer) {
	snd_pcm_writei(pcm, buffer, 4096);
	return 0;
}
