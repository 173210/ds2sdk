//av.c

#include "stdio.h"
#include "string.h"
//#include "nds_layer.h"
#include "av.h"

#define NDS_SCREEN_WIDTH    256
#define NDS_SCREEN_HEIGHT   192

#include "jz4740.h"
#include "test.h"

struct main_buf *pmain_buf;

void* up_screen_addr;
void* down_screen_addr;
//void* audio_buffer_addr;
static int up_screen_handle;
static int down_screen_handle;
static int audio_buffer_handle;

extern unsigned int audio_samples_per_trans;
extern unsigned int audio_samples_frequence;

/*
*   Initial video and audio interface between NDS
*       and clear the NDS screen
*/
int ds2io_init(int audio_samples_lenght)
{
    unsigned char *ptr;
    int buf_handle;

	audio_samples_lenght = (audio_samples_lenght >> 7) << 7;
	if(audio_samples_lenght > 4096) audio_samples_lenght = 4096;
	else if(audio_samples_lenght < 128) audio_samples_lenght = 1024;
	audio_samples_per_trans = audio_samples_lenght;

    test_main(0, NULL);

    buf_handle = get_video_up_buf();
    if(buf_handle < 0)
        return -1;

    ptr = (unsigned char*)get_buf_from_bufnum(buf_handle);
    memset(ptr, 0, NDS_SCREEN_WIDTH*NDS_SCREEN_HEIGHT*2);
    up_screen_addr = (void*)ptr;
    up_screen_handle = buf_handle;

    buf_handle = get_video_down_buf();
    if(buf_handle < 0)
        return -1;

    ptr = (unsigned char*)get_buf_from_bufnum(buf_handle);
    memset(ptr, 0, NDS_SCREEN_WIDTH*NDS_SCREEN_HEIGHT*2);
    down_screen_addr = (void*)ptr;
    down_screen_handle = buf_handle;

    ds2_flipScreen(DUAL, 1);

//	audio_buffer_handle = get_audio_buf();
//	if(audio_buffer_handle < 0)
//		return -1;

//	audio_buffer_addr = (void*)get_buf_from_bufnum(audio_buffer_handle);
//	memset(audio_buffer_addr, 0, audio_samples_per_trans*4);
	audio_buffer_handle = -1;

    printf("NDS Layer initial over\n");

    return 0;
}

/*
*   Initial video and audio interface between NDS
*       and clear the NDS screen
*	this is b version of ds2io_init
*/
int ds2io_initb(int audio_samples_lenght, int audio_samples_freq, int reserved1,
	int reserved2)
{
	reserved1 = reserved1;
	reserved2 = reserved2;
	audio_samples_frequence = (unsigned int)audio_samples_freq;
	return ( ds2io_init(audio_samples_lenght) );
}

/*
*   Flush data of video buffer to screen
*/
void ds2_flipScreen(enum SCREEN_ID screen_num, int done)
{
    unsigned int i;
    int buf_handle;

    if(UP == screen_num || DUAL == screen_num)
    {
        update_buf(up_screen_handle);

_do_up:
        buf_handle = get_video_up_buf();
        if(buf_handle >= 0)
        {
			if(2 == done) {
				while(check_video_up_buf());
				return;	//Not switch, for GUI convenience
			}

            up_screen_handle = buf_handle;
            up_screen_addr = (void*)get_buf_from_bufnum(buf_handle);
        } else if (done)
        {
			goto _do_up;
        }
    }

    if(DOWN == screen_num || DUAL == screen_num)
    {
        update_buf(down_screen_handle);

_do_down:
        buf_handle = get_video_down_buf();
        if(buf_handle >= 0)
        {
			if(2 == done) {
				while(check_video_down_buf());
				return;	//Not switch, for GUI convenience
			}

            down_screen_handle = buf_handle;
            down_screen_addr = (void*)get_buf_from_bufnum(buf_handle);
        } else if (done)
        {
			goto _do_down;
        }
    }
}

/*
*   Flush the screen to a single color
*/
void ds2_clearScreen(enum SCREEN_ID screen_num, unsigned short color)
{
    unsigned int i;
    unsigned int *dst; 
	unsigned int pixel;

	pixel = color + (color<<16);
    if(UP == screen_num || DUAL == screen_num)
    {
        dst= (unsigned int*)up_screen_addr;
        for(i= 0; i < NDS_SCREEN_WIDTH*NDS_SCREEN_HEIGHT/2; i++)
        *dst++ = pixel;
    }

    if(DOWN == screen_num || DUAL == screen_num)
    {
        dst= (unsigned int*)down_screen_addr;
        for(i= 0; i < NDS_SCREEN_WIDTH*NDS_SCREEN_HEIGHT/2; i++)
        *dst++ = pixel;
    }
}

int ds2_checkAudiobuff(void)
{
    int i;
    i= pmain_buf->nds_audio_w - pmain_buf->nds_audio_r;
    if (i>4 )
    {
	    dgprintf("audio err=%x\n",i);
    }

    return i;
}

void* ds2_getAudiobuff(void)
{
	unsigned int i;

    if (pmain_buf->buf_st_list[buf_audio_0].isused == 0)
        i = buf_audio_0;
    else if (pmain_buf->buf_st_list[buf_audio_1].isused == 0)
        i = buf_audio_1;
	else 
		return NULL;

	audio_buffer_handle = i;
	return ((void*)get_buf_from_bufnum(i));
}

void ds2_updateAudio(void)
{
	if(audio_buffer_handle < 0) return;

	update_buf(audio_buffer_handle);
	audio_buffer_handle = -1;
}

#if 0
void flip_audio(void)
{
	int handle;
	int ret;

	update_buf(audio_buffer_handle);

	while(1)
	{
		handle = get_audio_buf();
		if(handle >= 0) break;
	}

	audio_buffer_addr = (void*)get_buf_from_bufnum(handle);
	audio_buffer_handle = handle;
	return 0;
}
#endif

