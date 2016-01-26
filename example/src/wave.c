//wave.c

#include "ds2io.h"
#include "ds2_malloc.h"
#include "fs_api.h"
#include "wave.h"

int	wave_channle;
int wave_frequence;
int	wave_BytePsample;
int wave_length;

int check_waveheader(void *buffer)
{
	WaveHeader *wp = (WaveHeader*)buffer;

	if (wp->main_chunk == RIFF && wp->chunk_type == WAVE &&
		wp->sub_chunk == FMT && wp->data_chunk == DATA) {
	
		if (wp->format != PCM_CODE) {
			printf ("can't play not PCM-coded WAVE-files\n");
			return -1;
		}

	    if (wp->modus > 2) {
			printf ("can't play WAVE-files with %d tracks\n", wp->modus);
			return -1;
	    }

		wave_channle = wp->modus;
		wave_frequence = wp->sample_fq;
		wave_BytePsample = wp->byte_p_spl;
		wave_length = wp->data_length;
		
		printf("wave_channle: %d\n", wave_channle);
		printf("wave_frequence: %d\n", wave_frequence);
		printf("wave_BytePsample: %d\n", wave_BytePsample);
		printf("wave_bitPsamp: %d\n", wp->bit_p_spl);
		printf("wave_length: %d\n", wave_length);

		return 0;
	}

	return -1;
}

int doPlay(FILE *fp, int samples_per_trans)
{
	int len;
	int m, n;
	void *audiobuf;
	unsigned int key;
	int flag;
	int vol;
	unsigned short *audio_buffer_addr;

	len = wave_BytePsample * samples_per_trans;

	audiobuf = (void*)malloc(len);
	if(NULL == audiobuf) return -1;

	do {
		audio_buffer_addr = (unsigned short*)ds2_getAudiobuff();
	} while(NULL == audio_buffer_addr);

	vol = 127;

	flag = 1;
	while(flag)
	{
		n = fread(audiobuf, 1, len, fp);
		if(0 == n) break;

		m = (n + len -1)/len;
		m *= len;
		if(n < m)
		{
			memset((char*)((unsigned int)audiobuf + n), 0, m -n);
		}

		//assume 16-bit and stereo audio
   		unsigned short *src;
		unsigned short *dst0;
		unsigned short *dst1;

   		src = (unsigned short*)audiobuf;
		//the audio buffer's front part are left channel's data, behind ate right
		// channel's data, different as the wave file, it's data not interleaved
		dst0 = audio_buffer_addr;
        dst1 = audio_buffer_addr + samples_per_trans;

		n = 0;
		while(n++ < samples_per_trans)
		{
			*dst0++ = *src++;
			*dst1++ = *src++;
		}

		//ther are 4 audio buffers on the ds2sdk, we should not make it overflow
		while(ds2_checkAudiobuff() > 3)
		{
			//128 samples (channles * Bytes Per channel) can play 2.9ms @ 44.1KHz
			//mdelay((samples_per_trans/128)*2.9);
			mdelay(1);
		}

		ds2_updateAudio();

		while(ds2_checkAudiobuff() > 1)
		{
			key = getKey();
			
			switch(key)
			{
				//incread volume
				case KEY_UP:
						vol += 1;
						if(vol > 255) vol = 255;

						ds2_setVolume(vol);
						printf("volume: %d\n", vol);
					break;

				//decreade volume
				case KEY_DOWN:
						vol -= 1;
						if(vol < 0) vol = 0;

						ds2_setVolume(vol);
						printf("volume: %d\n", vol);
					break;

				//exit
				case KEY_B:
						flag = 0;
					break;
			}
		}

		do {
			audio_buffer_addr = (unsigned short*)ds2_getAudiobuff();
		} while(NULL == audio_buffer_addr);
	}

	free(audiobuf);
	return 0;
}

int play_wave(char *name)
{
	void *head;
	FILE *fp;
	int len;
	int flag;

	head = (unsigned int*)malloc(sizeof(WaveHeader));
	if(NULL == head)
		return -1;

	fp = fopen(name, "r");
	if(NULL == fp)
	{
		free(head);
		return -1;
	}

	len = fread(head, 1, sizeof(WaveHeader), fp);
	if(sizeof(WaveHeader) != len)
	{
		fclose(fp);
		free(head);
		return -1;
	}

	if(check_waveheader(head) == -1)
	{
		fclose(fp);
		free(head);
		return -1;
	}

	free(head);

	flag = doPlay(fp, audio_samples_per_trans);
	fclose(fp);

	return flag;
}


