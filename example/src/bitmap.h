#ifndef __BITMAP_H__
#define __BITMAP_H__

typedef struct _pixelmapheader{
	unsigned int		imHeadsize;	//Bitmap information header size
	unsigned int		imBitmapW;	//bitmap width in pixel
	unsigned int		imBitmapH;	//bitmap height in pixel
	unsigned short		imPlanes;	//bitmap planes numbers, must be set to 1
	unsigned short		imBitpixel;	//bits per pixel
	unsigned int		imCompess;	//compress method
	unsigned int		imImgsize;	//image size, times of 4-byte
	unsigned int		imHres;		//horizontal resolution, pixel/metel
	unsigned int		imVres;		//vertical resolution, pixel/metel
	unsigned int		imColnum;	//number of colors in color palette, 0 to exp(2)
	unsigned int		imImcolnum;	//important colors numbers used
} IMAGEHEADER;


typedef struct _bitmapfileheader{
	unsigned short		bfType;		//BMP file types
	unsigned int		bfSize;		//BMP file size(Not the pixel image size)
	unsigned short		bfReserved0;//reserved area0
	unsigned short		bfReserved1;//reserved area1
	unsigned int		bfImgoffst;	//pixel data area offset
	IMAGEHEADER bfImghead;
} BMPHEADER;

//#define NULL 0

//compression method
/* Value Identified by 	Compression method 		Comments
*	0 		BI_RGB 			none 				Most common
*	1 		BI_RLE8 		RLE 8-bit/pixel 	Can be used only with 8-bit/pixel bitmaps
*	2 		BI_RLE4 		RLE 4-bit/pixel 	Can be used only with 4-bit/pixel bitmaps
*	3 		BI_BITFIELDS 	Bit field 			Can be used only with 16 and 32-bit/pixel bitmaps.
*	4 		BI_JPEG 		JPEG 				The bitmap contains a JPEG image
*	5 		BI_PNG 			PNG 				The bitmap contains a PNG image
*/
#define BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#define BI_BITFIELDS 	3
#define BI_JPEG			4
#define BI_PNG			5

//error message
#define BMP_OK				0
#define BMP_ERR_OPENFAILURE	1
#define BMP_ERR_FORMATE		2
#define BMP_ERR_NOTSUPPORT	3
#define BMP_ERR_NEED_GO_ON	4


#define FILEOPENCHECK(fp)		(fp!=NULL)
		
extern int BMP_read(char* filename, char* buf, unsigned int *width, unsigned int *height, unsigned int* type);

#endif //__BITMAP_H__
