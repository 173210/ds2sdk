#include "stdio.h"
#include "bitmap.h"
#include "fs_api.h"

#ifndef u32
	#define u32 unsigned int
#endif

#ifndef s32
	#define s32 int
#endif

#ifndef u16
	#define u16 unsigned short
#endif

int BMP_read(char* filename, char* buf, unsigned int *width, unsigned int *height, unsigned int* type)
{
	FILE* fp;
	BMPHEADER bmp_header;
	int	flag;
	u32 bytepixel;
	u32	x, y, sx, sy, m, len;
	unsigned char *dest;
	s32	fpos;
	unsigned char st[54];
	
	fp= fopen(filename, "rb");

    if(fp == NULL)
		return BMP_ERR_OPENFAILURE;
	
	flag= fread(st, sizeof(st), 1, fp);
	if(!flag)
	{
		fclose(fp);
		return BMP_ERR_FORMATE;
	}
	
    bmp_header.bfType= *((u16*)st);
    bmp_header.bfSize= *((u16*)(st+2)) | *((u16*)(st+4));
    bmp_header.bfReserved0= *((u16*)(st+6));
    bmp_header.bfReserved1= *((u16*)(st+8));
    bmp_header.bfImgoffst= *((u16*)(st+10)) | *((u16*)(st+12));
    bmp_header.bfImghead.imHeadsize= *((u16*)(st+14)) | *((u16*)(st+16));
	bmp_header.bfImghead.imBitmapW= *((u16*)(st+18)) | *((u16*)(st+20));
	bmp_header.bfImghead.imBitmapH= *((u16*)(st+22)) | *((u16*)(st+24));
	bmp_header.bfImghead.imPlanes= *((u16*)(st+26));
	bmp_header.bfImghead.imBitpixel= *((u16*)(st+28));
	bmp_header.bfImghead.imCompess= *((u16*)(st+30)) | *((u16*)(st+32));
	bmp_header.bfImghead.imImgsize= *((u16*)(st+34)) | *((u16*)(st+36));
	bmp_header.bfImghead.imHres= *((u16*)(st+38)) | *((u16*)(st+40));
	bmp_header.bfImghead.imVres= *((u16*)(st+42)) | *((u16*)(st+44));
	bmp_header.bfImghead.imColnum= *((u16*)(st+46)) | *((u16*)(st+48));
	bmp_header.bfImghead.imImcolnum= *((u16*)(st+50)) | *((u16*)(st+52));

    
	if(bmp_header.bfType != 0x4D42)	//"BM"
	{
		fclose(fp);
		return BMP_ERR_FORMATE;
	}

	if(bmp_header.bfImghead.imCompess != BI_RGB && 
		bmp_header.bfImghead.imCompess != BI_BITFIELDS)
	{
		fclose(fp);
		return BMP_ERR_NEED_GO_ON;		//This funciton now not support...
	}

	bytepixel= bmp_header.bfImghead.imBitpixel >> 3;
	if(bytepixel < 2)					//byte per pixel >= 2
	{
		fclose(fp);
		return BMP_ERR_NEED_GO_ON;		//This funciton now not support...
	}

	*type = bytepixel;

	x= *width;
	y= *height;
	sx= bmp_header.bfImghead.imBitmapW;
	sy= bmp_header.bfImghead.imBitmapH;

	len = x < sx ? x : sx;
	if(y > sy)
		y= sy;

	//BMP scan from down to up
	fpos= (s32)bmp_header.bfImgoffst;
	dest= (unsigned char*)buf+(y-1)*x*4;
	for(m= 0; m < y; m++) {
		fseek(fp, fpos, SEEK_SET);
		fread(dest, 1, len*bytepixel, fp);
		fpos += ((sx*bytepixel+3)>>2)<<2;
		dest -= x*4;
	}

	fclose(fp);

	if(x > sx)
		x= sx;

	*width = x;
	*height= y;	

	//Only read out pixels, other things remain to the caller
/*
	if(bytepixel== 3)
	{
		unsigned char *pt1, *pt2;
		
		dest= (unsigned char*)buf;
		for(m= 0; m < y; m++) {
			dest += x*4;
			pt1= dest-x-1;
			pt2= dest-1;
			for(sy= 0; sy < x; sy++) {
				*pt2--= 255;
				*pt2-- = *pt1--;
				*pt2-- = *pt1--;
				*pt2-- = *pt1--;
			}
		}
	}
*/

	return BMP_OK;
}

