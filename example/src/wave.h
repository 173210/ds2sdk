#ifndef __WAVE_H__
#define __WAVE_H__

/* Definitions for .VOC files */

#define MAGIC_STRING	"Creative Voice File\x1A"
#define ACTUAL_VERSION	0x010A
#define VOC_SAMPLESIZE	8

#define MODE_MONO	0
#define MODE_STEREO	1

#define DATALEN(bp)	((unsigned int)(bp->datalen) | \
                         ((unsigned int)(bp->datalen_m) << 8) | \
                         ((unsigned int)(bp->datalen_h) << 16) )

typedef struct _vocheader {
  unsigned char  magic[20];				//must be MAGIC_STRING
  unsigned short headerlen;				//Headerlength, should be 0x1A
  unsigned short version;				//VOC-file version
  unsigned short coded_ver;				//0x1233-version
} VocHeader;

typedef struct _blocktype {
  unsigned char  type;
  unsigned char  datalen;				//low-byte
  unsigned char  datalen_m;				//medium-byte
  unsigned char  datalen_h;				//high-byte
} BlockType;

typedef struct _voice_data {
  unsigned char  tc;
  unsigned char  pack;
} Voice_data;

typedef struct _ext_block {
  unsigned short tc;
  unsigned char  pack;
  unsigned char  mode;
} Ext_Block;


/* Definitions for Microsoft WAVE format */

#define RIFF		0x46464952	
#define WAVE		0x45564157
#define FMT			0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

typedef struct _waveheader {
	unsigned int	main_chunk;		//'RIFF'
	unsigned int	length;			//filelen
	unsigned int	chunk_type;		//'WAVE'

	unsigned int	sub_chunk;		//'fmt '
	unsigned int	sc_len;			//length of sub_chunk, =16
	unsigned short	format;			//should be 1 for PCM-code
	unsigned short	modus;			//1 Mono, 2 Stereo
	unsigned int	sample_fq;		//frequence of sample
	unsigned int	byte_p_sec;
	unsigned short	byte_p_spl;		//samplesize; 1 or 2 bytes
	unsigned short	bit_p_spl;		//8, 12 or 16 bit

	unsigned int	data_chunk;		//'data'
	unsigned int	data_length;	//samplecount
} WaveHeader;

typedef struct {
    int magic;						//must be equal to SND_MAGIC
    int dataLocation;				//Offset or pointer to the raw data
    int dataSize;					//Number of bytes of data in the raw data
    int dataFormat;					//The data formate coding
    int samplingRate;				//The sampling rate
    int channelCount;				//The number of channels
} SndHeader;

#define SND_MAGIC ((long int)0x2e736e64)

#define SND_FORMAT_UNSPECIFIED          (0)
#define SND_FORMAT_MULAW_8              (1)
#define SND_FORMAT_LINEAR_8             (2)
#define SND_FORMAT_LINEAR_16            (3)
#define SND_FORMAT_LINEAR_24            (4)
#define SND_FORMAT_LINEAR_32            (5)
#define SND_FORMAT_FLOAT                (6)

#endif //__WAVE_H__

