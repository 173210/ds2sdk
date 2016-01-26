//av.h
#ifndef __AV_H__
#define __AV_H__


typedef enum SCREEN_ID
{
    UP = 1,
    DOWN = 2,
    DUAL = 3
} SCREEN_ID;

#define UP_MASK		0x1
#define DOWN_MASK	0x2
#define DUAL_MASK	0x3

#define SCREEN_WIDTH	256
#define SCREEN_HEIGHT	192


extern void* up_screen_addr;
extern void* down_screen_addr;

extern int ds2io_init(int audio_samples_lenght);

extern void ds2_flipScreen(enum SCREEN_ID screen_num, int done);
//extern void flip_screen(enum SCREEN_ID screen_num, int done);

extern void clear_screen(enum SCREEN_ID screen_num, unsigned short color);
//extern void ds2_clearScreen(enum SCREEN_ID screen_num, unsigned short color);

#endif //__AV_H__

