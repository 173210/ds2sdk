#ifndef _DEV_VAR_H
#define _DEV_VAR_H

#define MP4_MAJOR 88
#define MP4_MINOR 88

#ifdef ENABLE_TYPEDEFBASE

typedef     unsigned char           u8;
typedef     unsigned short int      u16;
typedef     unsigned int            u32;
typedef     unsigned long long int  u64;

typedef     signed char             s8;
typedef     signed short int        s16;
typedef     signed int              s32;
typedef     signed long long int    s64;

#endif


#ifdef ENABLE_TYPEDEF
typedef     float                   f32;
typedef     double                  f64;


#define     vl                      volatile

typedef     vl u8                   vu8;
typedef     vl u16                  vu16;
typedef     vl u32                  vu32;
typedef     vl u64                  vu64;

typedef     vl s8                   vs8;
typedef     vl s16                  vs16;
typedef     vl s32                  vs32;
typedef     vl s64                  vs64;

typedef     vl f32                  vf32;
typedef     vl f64                  vf64;
#endif

#ifndef ARM9
//设备结构
/*
struct MP4_dev 
{
	struct semaphore sem;     
	wait_queue_head_t wq_read;
	wait_queue_head_t wq_write;
	struct cdev cdev;	 
};
*/
#endif
enum {
	SET_CTR = 1,
	CLR_CTR,
	READ_STATE,
	READ_IO,
	WRITE_IO,
	IQE_WRITE,
	IQE_READ,
	READ_KEY,
	WRITE_BUF,
	READ_BUF,
    IQE_UPDATE,
	COMPLETE_STATE,
    ENABLE_FIXRGB_UPVIDEO,
    ENABLE_FIXRGB_DOWNVIDEO,
    Homebrew_SD_READ_END,
    Homebrew_SD_WRITE_END



};
#ifndef ARM9
typedef enum KEYPAD_BITS {
  KEY_A      = BIT(0),  //!< Keypad A button.
  KEY_B      = BIT(1),  //!< Keypad B button.
  KEY_SELECT = BIT(2),  //!< Keypad SELECT button.
  KEY_START  = BIT(3),  //!< Keypad START button.
  KEY_RIGHT  = BIT(4),  //!< Keypad RIGHT button.
  KEY_LEFT   = BIT(5),  //!< Keypad LEFT button.
  KEY_UP     = BIT(6),  //!< Keypad UP button.
  KEY_DOWN   = BIT(7),  //!< Keypad DOWN button.
  KEY_R      = BIT(8),  //!< Right shoulder button.
  KEY_L      = BIT(9),  //!< Left shoulder button.
  KEY_X      = BIT(10), //!< Keypad X button.
  KEY_Y      = BIT(11), //!< Keypad Y button.
  KEY_TOUCH  = BIT(12), //!< Touchscreen pendown.
  KEY_LID    = BIT(13)  //!< Lid state.
} KEYPAD_BITS;

typedef struct touchPosition {
	s16	x;
	s16	y;
} touchPosition;

#endif

enum {
	buf_video_up_0= 0,
	buf_video_up_1,
	buf_video_down_0,
	buf_video_down_1,
    buf_audio_0,
    buf_audio_1,
    buf_c0_set,
	buf_video_up_3,
	buf_video_up_4,



    buf_max_num

};
enum {
        VIDEO_UP = 1,
        VIDEO_DOWN,
        AUDIO_G,
        VIDEO_UP_TXT 

};
enum {
        IS_SET_VIDEO=1,
        IS_SET_AUDIO,
        IS_SET_ENABLE_VIDEO,
        IS_SET_ENABLE_AUDIO,
        IS_SET_ENABLE_KEY,
        IS_SET_ENABLE_RTC,
        IS_SET_CLEAR_VAR,

        IS_SET_VOL,
        IS_SET_BR,
        IS_SET_SWAP,
        IS_SET_BACKLIGHT,

        IS_SET_SUSPEND,
        IS_SET_WAKEUP,
        IS_SET_SHUTDOWN,
        IS_WRITE_IO,

        IS_RUN_Homebrew,
        IS_RUN_JZHomebrew

};
struct nds_iqe_st 
{
    u32 iqe_cmd;
    u32 iqe_ndsaddr;
    u32 iqe_cpuaddr;
    u32 iqe_datatype;
    u32 iqe_len;
    u32 iqe_num;
};

struct nds_iqe_list_st
{
int num;
u32 c0_addr;
};


struct buf_st 
{
    u32 isused;
    u32 offset;
    u32 len;
    u32 use_len;
    u32 nds_max_len;
    u32 nds_cmd;
    u32 type;
    //u32 load_offset;

}; 

struct rtc{
    vu8 year;		//add 2000 to get 4 digit year
    vu8 month;		//1 to 12
    vu8 day;		//1 to (days in month)

    vu8 weekday;	// day of week
    vu8 hours;		//0 to 11 for AM, 52 to 63 for PM
    vu8 minutes;	//0 to 59
    vu8 seconds;	//0 to 59
} ;
struct key_buf 
{
    u16 key;
    u16 x;
    u16 y;
    u16 brightness;

};
struct audio_set_vol
{
    int l_vol;
    int r_vol;
    int mix_vol;
};

struct main_buf
{
    u32 key_buf_offset;
    u32 key_buf_len;
    u32 key_write_num;
    u32 key_read_num;

    struct buf_st buf_st_list[0x10];
    u32 tempbuff;
    u32 tempbuff_len;

    u32 nds_video_up_w;
    u32 nds_video_up_r;
    u32 nds_video_down_w;
    u32 nds_video_down_r;
    u32 nds_audio_w;
    u32 nds_audio_r;
    struct rtc nds_rtc;

    u32 nds_iqe_list_c0[0x10][512/4];

    u32 nds_Homebrew_tempbuf[512/4];
    u32 nds_Homebrew_addr;
    u32 nds_Homebrew_cmd;


};

struct video_set
{
    int frequence;
    int timer_l_data;
    int timer_l_ctr;
    int timer_h_data;
    int timer_h_ctr;
    int width;
    int height;
    int data_type;
    int play_buf;//a=0 b=1
    int swap;//0 a up

};
struct audio_set
{
    int sample;
    int timer_l_data;
    int timer_l_ctr;
    int timer_h_data;
    int timer_h_ctr;
    int sample_size;
    int sample_bit;
    int data_type;
    int stereo;
};
///////////////////////////////////////////////////////////////
typedef struct T_INPUT
{
    u32 keysHeld;
    u32 keysUp;
    u32 keysDown;
    u32 keysDownRepeat;
    touchPosition touchPt;
    touchPosition movedPt;
    int touchDown;
    int touchUp;
    int touchHeld;
    int touchMoved;
}INPUT;

enum {
        LOAD_FLAG_OK = 1,
        LOAD_FLAG_SD_NO_INSERT,
        LOAD_FLAG_SD_UNKNOW,
        LOAD_FLAG_SD_NO_NDS_FILE,
        LOAD_FLAG_SD_NO_LINUX_FILE,
        LOAD_FLAG_FIND_UPGRADE,
        LOAD_FLAG_UPGRADE_WAIT,
        LOAD_FLAG_UPGRADE_ERROR_LANGUAGE,
        LOAD_FLAG_UPGRADE_ERROR_WRITE_FLASH,
        LOAD_FLAG_UPGRADE_OK

};
#ifndef ARM9
//typedef     int                  bool;
#endif


//typedef enum {cmd_normal = 0,key1_cmd_enable,key2_cmd_enable} NDS_CMD_STATE;
//typedef enum {data_normal = 0,key2_data_enable} NDS_DATA_STATE;
typedef enum {
    CMD_NORMAL = 0,
    KEY1_CMD_ENABLE,
    KEY2_CMD_ENABLE
} NDS_CMD_STATE;

typedef enum {
    DATA_NORMAL = 0,
    KEY2_DATA_ENABLE
} NDS_DATA_STATE;

typedef enum {NDS_LOAD = 0,NDSI_LOAD,LINUX_LOAD} NDS_LOAD_STATE;

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
typedef struct fix_fat_st
{
    u32 s_Cluster;
    u32 d_Cluster;
    u32 next_addr;
}fix_fat_st;


typedef struct cpu_fs {
    u32 startCluster;
    u32* fat_addr;
    u32 bytesPerCluster;
    u32 file_len;
    u32 mask_len;
    int fat_addr_len;
    //FILE *fp;
    char filename[256];

} cpu_fs;


/*
typedef struct rominfo {
    u32 startCluster;
    u32* fat_addr;
    u32 fat_fix_addr;
    u32 file_len;
    u32 mask_len;
    int fat_addr_len;
    //FILE *fp;
    char filename[256];

} rominfo;
*/

typedef struct patchrominfodat {
  u32 SCGamecrc32;
  u32 SCGameCode;
  u32 offset;

} patchrominfodat;



typedef struct rominfodata {
s32 SCGameCrc;
u32 SCGameCode;
s32 SCGameSaverSize;
u8 save_init_var;
u8 enable_patch;
u8 enable_cheat;
u8 data[32 -(4*3) -2];

} rominfodata;

enum {
CPU_CMD_EXIT=1,
CPU_CMD_END,
CPU_CMD_WRITE_VIDEO,
CPU_CMD_WRITE_AUDIO,
CPU_CMD_WRITE_SET,
//CPU_CMD_READ_KEY


};
enum {
ret_flag_return_game=1,
ret_flag_load_game,
ret_flag_save_game,
ret_flag_load_cheat,
ret_flag_reset

};
enum {
REALTIME_SET_AUDIO=1,
REALTIME_SET_AUDIO_ENABLE,
REALTIME_SET_BRIGHTNESS,
REALTIME_SET_KEY,
REALTIME_SET_SLOW,

};
enum {
    request_flag_set=0,
    request_flag_audio,
    request_flag_video,
//    request_flag_key,
    request_flag_end,
    request_flag_max,
};
typedef struct TPatchInfo{
    int gamename_offset;
    int saver_size;
    int saver_num;
    int enable_cheat;
    int enable_patch;
    int save_init_var;
		int slowPercent;
    u32 freebuf[2];
} TPatchInfo;

/*
typedef struct TPatchInfo{
    int gamename_offset;
    int saver_size;
    int saver_num;
    u8 enable_cheat;
    u8 enable_patch;
    u8 save_init_var;
		u8 slowPercent;
    u8 enable_clear_run;
    u8 saver_mode;
    u8 reserved[64-18];
    u32 freebuf[2];
} TPatchInfo;
*/
/*
typedef struct pacth_var{
int enable_resume;        //arm9 0x8000 前的补丁复原
u32 resume_data[32][2];//[0] addr ,[1] data
u32 read_dma[2];

} pacth_var;
*/
#ifdef NDS
  #ifdef DS_TWO_DEBUG
    #ifdef ARM9
    void wait_press_b()
    {
        dgprintf("\npress B to continue.\n");
        scanKeys();
        u16 keys_up = 0;
        while( 0 == (keys_up & KEY_B) )
        {
            scanKeys();
            keys_up = keysUp();
        }
    }
    #endif
  #else
    #define wait_press_b()
  #endif
#endif




typedef struct flash_index_table{
int key_addr;
int key_len;
int ndsl_head_addr;
int ndsl_head_len;
int ndsl_rom_offset;
int ndsl_data_addr;
int ndsl_data_len;
int ndsi_rom_index_addr;
int ndsi_rom_index_len;
int ndsi_rom_data_addr;
int ndsi_rom_data_len;

int set[(0x80/4) - 4 - 11];
int updata_flag;
int lg;
int ver;
int clk;
} flash_index_table;


typedef struct ndsi_index_struct{
int rom_begin_offset;
int rom_end_offset;
int flash_offset;
} ndsi_index_struct;

enum {
    nds_boot_exit_dstwo_gui=0,
    nds_boot_exit_dstwo_test,
    nds_boot_exit_dstwo_plugin,
};



#endif /*__cpu_nds_H__ */


