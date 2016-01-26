//test.c

#include "jz4740.h"
#include "test.h"

typedef enum SCREEN_ID
{
    UP_SCREEN = 1,
    DOWN_SCREEN = 2,
    DUAL_SCREEN = 3
} SCREEN_ID;

extern u32 kmalloc_ptr_uncache;

static void _initInput(void);

unsigned short __brightness_state;

struct main_buf *pmain_buf;
int MP4_fd;
int MP4_buf;

u32 get_buf_from_bufnum(int num)
{
    return (pmain_buf->buf_st_list[num].offset + MP4_buf);
}

int check_video_up_buf(void)
{
    int i = 0;

	if (pmain_buf->buf_st_list[buf_video_up_0].isused != 0) i += 1;
    if (pmain_buf->buf_st_list[buf_video_up_1].isused != 0) i += 1;

	return i;
}

int get_video_up_buf(void)
{
    int ret = -1;
    if (pmain_buf->buf_st_list[buf_video_up_0].isused == 0)
    {
        ret= buf_video_up_0;
    }
    if (pmain_buf->buf_st_list[buf_video_up_1].isused == 0)
    {
        ret= buf_video_up_1;
    }

    return ret;
}

int check_video_down_buf(void)
{
    int i= 0;

    if (pmain_buf->buf_st_list[buf_video_down_0].isused != 0) i += 1;
    if (pmain_buf->buf_st_list[buf_video_down_1].isused != 0) i += 1;
	
	return i;
}

int get_video_down_buf(void)
{
    if (pmain_buf->buf_st_list[buf_video_down_0].isused == 0)
    {
        return buf_video_down_0;
    }
    if (pmain_buf->buf_st_list[buf_video_down_1].isused == 0)
    {
        return buf_video_down_1;
    }
    return -1;
}

#if 0
int nds_buf_audio_notfull(void)
{
    int i;
    i=pmain_buf->nds_audio_w - pmain_buf->nds_audio_r;
    if (i>4 )
    {
    dgprintf("audio err=%x\n",i);

    }

    return i;
}
#endif

int get_audio_buf(void)
{
    if (pmain_buf->buf_st_list[buf_audio_0].isused == 0)
    {
        return buf_audio_0;
    }
    if (pmain_buf->buf_st_list[buf_audio_1].isused == 0)
    {
        return buf_audio_1;
    }
    
    return -1;
}

int get_nds_set_buf(void)
{
    if (pmain_buf->buf_st_list[buf_c0_set].isused == 0)
    {
        return buf_c0_set;
    }

    return -1;
}

int update_buf(int bufnum)
{
	int ret = -1;
	if (pmain_buf->buf_st_list[bufnum].isused == 0)
	{
		ioctl(MP4_fd, IQE_UPDATE, bufnum);
		ret = 0;
	}

	return ret;
}

/*
*	Functin: get time
*/
void ds2_getTime(struct rtc *time)
{
    time->year= pmain_buf->nds_rtc.year;
    time->month= pmain_buf->nds_rtc.month;
    time->day= pmain_buf->nds_rtc.day;
    time->weekday= pmain_buf->nds_rtc.weekday;
    time->hours= pmain_buf->nds_rtc.hours;
    time->minutes= pmain_buf->nds_rtc.minutes;
    time->seconds= pmain_buf->nds_rtc.seconds;
}

int set_nds_var(u32* data, int len)
{
    u32 *temp32p;
    int buf_num, flag, flag1;

    flag = 0;
    while (flag == 0)
    {
        buf_num = get_nds_set_buf();
        if (buf_num >= 0) break;
    }
    temp32p = (u32*)(pmain_buf->buf_st_list[buf_num].offset + MP4_buf);

    memset(temp32p, 0, 512);

	if(len > 512) len = 512;
    memcpy(temp32p, data, len);

    flag = 0;
    while (flag == 0)
    {
        flag1 = update_buf(buf_num);
        if (flag1 >= 0) break;
    }

    return 0;
}

/*
*	Function: get brightness of the screens
*/
int ds2_getBrightness(void)
{
//    return curr_input.brightness &3;
    return __brightness_state & 3;
}

/*
*	Function: set brightness of the screens
*	Input: level, there are 4 levels, 0, 1, 2 and 3
*/
void ds2_setBrightness(int level)
{
    u32 tempbuf12[512/4];
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_BR;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=level & 3;

    set_nds_var(tempbuf12,512);
}

/*
*	Function: get the swaping state of the screens
*/
int ds2_getSwap(void)
{
//    return (curr_input.brightness>>2) &1;
	return (__brightness_state>>2) &1;
}

/*
*	Funciotn: swap up screen and down screen
*/
void ds2_setSwap(int swap)
{
    u32 tempbuf12[512/4];

    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_SWAP;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=swap & 1;

    set_nds_var(tempbuf12,512);
}

/*
*	Function: get backlight status
*	return bit0 = 0 the down screen's backlight is off
*			bit0 = 1 the down screen's backlight is on
*			bit1 = 0 the up screen's backlight is off
*			bit1 = 1 the up screen's backlight is on
*/
void ds2_setBacklight(int backlight)
{
    u32 tempbuf12[512/4];

    memset(tempbuf12, 0, 512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_BACKLIGHT;
    tempbuf12[2]=0x60;
//    tempbuf12[0x60/4]=((down_st & 1) | ((up_st & 1) <<1));
    tempbuf12[0x60/4]= backlight & 3;

    set_nds_var(tempbuf12,512);
}

/*
*	Function: get backlight status
*	return bit0 = 0 the down screen's backlight is off
*			bit0 = 1 the down screen's backlight is on
*			bit1 = 0 the up screen's backlight is off
*			bit1 = 1 the up screen's backlight is on
*/
int ds2_getBacklight(void)
{
//	if(screen & UP_SCREEN)
//	    return (curr_input.brightness>>4) &1;
//		return (__brightness_state >> 4) &1;
//	else
//		return (curr_input.brightness>>3) &1;
//		return (__brightness_state >> 3) &1;
	return (__brightness_state >> 3) & 0x3;
}

/*
*	Function: system suspend
*/
void ds2_setSupend(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_SUSPEND;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;

    set_nds_var(tempbuf12,512);
}

/*
*	Function: system wakeup
*/
void ds2_wakeup(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_WAKEUP;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;
    set_nds_var(tempbuf12,512);
}

/*
*	Function: NDS power offf
*/
void ds2_shutdown(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
            break;
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_SHUTDOWN;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;

    set_nds_var(tempbuf12,512);
    while(1);
}

/*
*	Function: set volume of NDS
*/
void ds2_setVolume(int mix_vol)
//void ds2_setVolume(int l_vol, int r_vol, int mix_vol)
{
    struct audio_set_vol *audio_set_volp;
    u32 tempbuf12[512/4];

    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_VOL;
    tempbuf12[2]=0x60;
    audio_set_volp =(struct audio_set_vol *)&tempbuf12[0x60/4];
    audio_set_volp->l_vol= 0x7F;//l_vol;
    audio_set_volp->r_vol= 0x7F;//r_vol;
    audio_set_volp->mix_vol=mix_vol;
    set_nds_var(tempbuf12,512);
}

#if 0
int pause_play(void)
{
    int flag, flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_ENABLE_VIDEO;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=0;

    tempbuf12[3]=IS_SET_ENABLE_AUDIO;
    tempbuf12[4]=0x64;
    tempbuf12[0x64/4]=0;
    set_nds_var(tempbuf12,512);
	return 0;
}
#endif

#if 0
int begin_pause_play(void)
{
    int flag, flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_ENABLE_VIDEO;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;

    tempbuf12[3]=IS_SET_ENABLE_AUDIO;
    tempbuf12[4]=0x64;
    tempbuf12[0x64/4]=1;
    set_nds_var(tempbuf12,512);

    return 0;
}
#endif

static void _switch_on_nds(void)
{
    int flag,flag1;
	u32 *temp32p;
    int offset;
    int offsetindex;
    u32 tempbuf12[512/4];

    flag1=0;
	while (flag1 == 0)
    {
		ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)			break;
	}

	pmain_buf->nds_video_up_w=0;
	pmain_buf->nds_video_up_r=0;

	pmain_buf->nds_video_down_w=0;
	pmain_buf->nds_video_down_r=0;

	pmain_buf->nds_audio_w=0;
	pmain_buf->nds_audio_r=0;
	pmain_buf->key_write_num=0;
	pmain_buf->key_read_num=0;
	
	int i;
	for(i= 0; i< 0x10; i++)
	{
		pmain_buf->buf_st_list[i].isused  =0;
	}

    memset(tempbuf12,0,512);
	temp32p=tempbuf12;

	temp32p[0]=0xc0;
    offset = 0x60;
	offsetindex=0x4;

    temp32p[offsetindex/4]=IS_SET_CLEAR_VAR;offsetindex+=4;
	temp32p[offsetindex/4]=offset;offsetindex+=4;
	temp32p[offset/4]=1;
	offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_VIDEO;offsetindex+=4;
	temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;


    temp32p[offsetindex/4]=IS_SET_ENABLE_AUDIO;offsetindex+=4;
	temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
	offset+=4;

	temp32p[offsetindex/4]=IS_SET_ENABLE_KEY;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;

	temp32p[offsetindex/4]=IS_SET_ENABLE_RTC;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
	offset+=4;

	set_nds_var(tempbuf12,512);
}



int test_main(int argc, char **argv)
{
	dgprintf("Initial NDS layer\n");
    MP4_init_module();

    dgprintf("Module initialed\n");

    //get buff
	MP4_buf = kmalloc_ptr_uncache;
    pmain_buf=(struct main_buf*)MP4_buf;

    _switch_on_nds();

#if 0
	dgprintf("MP4_buf =%x",MP4_buf);
    dgprintf("pmain_buf[0] =%x,%x\n",pmain_buf->buf_st_list[0].offset,pmain_buf->buf_st_list[0].len);
    dgprintf("pmain_buf[1] =%x,%x\n",pmain_buf->buf_st_list[1].offset,pmain_buf->buf_st_list[1].len);
    dgprintf("pmain_buf[2] =%x,%x\n",pmain_buf->buf_st_list[2].offset,pmain_buf->buf_st_list[2].len);
    dgprintf("pmain_buf[3] =%x,%x\n",pmain_buf->buf_st_list[3].offset,pmain_buf->buf_st_list[3].len);
    dgprintf("pmain_buf[4] =%x,%x\n",pmain_buf->buf_st_list[4].offset,pmain_buf->buf_st_list[4].len);
    dgprintf("pmain_buf[5] =%x,%x\n",pmain_buf->buf_st_list[5].offset,pmain_buf->buf_st_list[5].len);
    dgprintf("pmain_buf[6] =%x,%x\n",pmain_buf->buf_st_list[6].offset,pmain_buf->buf_st_list[6].len);
    dgprintf("nds_video_up =%x,%x\n",pmain_buf->nds_video_up_w,pmain_buf->nds_video_up_r);
    dgprintf("nds_video_down =%x,%x\n",pmain_buf->nds_video_down_w,pmain_buf->nds_video_down_r);
    dgprintf("nds_audio =%x,%x\n",pmain_buf->nds_audio_w,pmain_buf->nds_audio_r);
#endif

	_initInput();

    return 0;
}


int getmm1_ok(void)
{
//    return (curr_input.brightness >>6) & 1;
    return (__brightness_state >>6) & 1;
}


int getmm2_ok(void)
{
//    return (curr_input.brightness >>7) & 1;
    return (__brightness_state >>7) & 1;
}

static struct key_buf __last_input;

void _initInput(void)
{
    pmain_buf->key_read_num=pmain_buf->key_write_num= 0;
    memset((char*)&__last_input, 0, sizeof(__last_input));
	__brightness_state = 0;
}

/*
*	Funciton: get key value and touch screen position value
*/
void ds2_getrawInput(struct key_buf *input)
{
	struct key_buf *keyp;
	int i;

    i = pmain_buf -> key_write_num - pmain_buf -> key_read_num;
	if(i > 1)
		pmain_buf -> key_read_num = pmain_buf -> key_write_num -1;

    keyp =(struct key_buf*)(MP4_buf + pmain_buf -> key_buf_offset +
		((pmain_buf -> key_read_num & KEY_MASK) * sizeof(struct key_buf)));

    memcpy(input, keyp, sizeof(struct key_buf));
}





