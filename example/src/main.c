//main.c

#include <stdio.h>

#include "ds2io.h"
#include "ds2_cpu.h"
#include "ds2_malloc.h"
#include "fs_api.h"
#include "key.h"

//#define MAX_PATH	256

#define RGB24_15(pixel) ((((*pixel) & 0xF8) << 7) |\
                        (((*(pixel+1)) & 0xF8) << 2) |\
                        (((*(pixel+2)) & 0xF8)>>3))

#define RGB16_15(pixel) ((((*pixel)>>10) & 0x1F) |\
						(((*pixel) & 0x1F) << 10) |\
						((*pixel) & 0x83E0))

#define BLACK_COLOR		RGB15(0, 0, 0)
#define WHITE_COLOR		RGB15(31, 31, 31)
#define GREEN_COLOR		RGB15(0, 31, 0)

struct  FILE_LIST_INFO
{
    char current_path[MAX_PATH];
    char **wildcards;
    unsigned int file_num;
    unsigned int dir_num;
	unsigned int mem_size;
	unsigned int file_size;
	unsigned int dir_size;
    char **file_list;
    char **dir_list;
    char *filename_mem;
};


#define STATUS_ROWS 0
#define CURRENT_DIR_ROWS 2
#define FILE_LIST_ROWS 23
#define FILE_LIST_ROWS_CENTER   (FILE_LIST_ROWS/2)
#define FILE_LIST_POSITION 42
//#define DIR_LIST_POSITION 360
#define DIR_LIST_POSITION 130
#define PAGE_SCROLL_NUM 7
#define SUBMENU_ROW_NUM	5




static int sort_function(const void *dest_str_ptr, const void *src_str_ptr)
{
  char *dest_str = *((char **)dest_str_ptr);
  char *src_str = *((char **)src_str_ptr);
 
  if(src_str[0] == '.')
    return 1;

  if(dest_str[0] == '.')
    return -1;

  return strcasecmp(dest_str, src_str);
}

static int my_array_partion(void *array, int left, int right)
{
    unsigned int pivot= *((unsigned int*)array + left);

    while(left < right)
    {
        while(sort_function((void*)((unsigned int*)array+left), (void*)((unsigned int *)array+right)) < 0) {
            right--;
        }

        if(right== left) break;
        *((unsigned int*)array + left) = *((unsigned int*)array + right);
        *((unsigned int*)array + right) = pivot;

        if(left < right)
        {
            left++;
            if(right== left) break;
        }

        while(sort_function((void*)((unsigned int*)array+right), (void*)((unsigned int *)array+left)) > 0) {
            left++;
        }

        if(left== right) break;
        *((unsigned int*)array + right) = *((unsigned int*)array + left);
        *((unsigned int*)array + left) = pivot;
        right--;
    }

    return left;
}

void my_qsort(void *array, int left, int right)
{
    if(left < right)
    {
        int mid= my_array_partion(array, left, right);
        my_qsort(array, left, mid-1);
        my_qsort(array, mid+1, right);
    }
}

static void strupr(char *str)
{
    while(*str)
    {
        if(*str <= 0x7A && *str >= 0x61) *str -= 0x20;
        str++;
    }
}

//******************************************************************************
//flag: 0 initialize
//		1 increase filename memory size
//		2 increase file name buffer size
//		4 increase directory name buffer size
//		-1 free all allocated memroy
//		other invalide
//return: -1 on failure
//******************************************************************************
#define FILE_LIST_MAX  256
#define DIR_LIST_MAX   64
#define NAME_MEM_SIZE  (320*64)
int manage_filelist_info(struct FILE_LIST_INFO *filelist_infop, int flag)
{
	//Initialize
	if(0 == flag)
	{
		filelist_infop->file_list = (char**)malloc(FILE_LIST_MAX*4);
		if(NULL == filelist_infop->file_list)
			return -1;

		filelist_infop->dir_list = (char**)malloc(DIR_LIST_MAX*4);
		if(NULL == filelist_infop->dir_list)
		{
			free((void*)filelist_infop->file_list);
			return -1;
		}

		filelist_infop->filename_mem = (char*)malloc(NAME_MEM_SIZE);
		if(NULL == filelist_infop->filename_mem)
		{
			free((void*)filelist_infop->file_list);
			free((void*)filelist_infop->dir_list);
			return -1;
		}

		filelist_infop->mem_size = NAME_MEM_SIZE;
		filelist_infop->file_size = FILE_LIST_MAX;
		filelist_infop->dir_size = DIR_LIST_MAX;
		return 0;
	}
	//free all memroy
	if(-1 == flag)
	{
		free((void*)filelist_infop->file_list);
		free((void*)filelist_infop->dir_list);
		free((void*)filelist_infop->filename_mem);
		return 0;
	}

	int i;
	void *pt;

	//Increase all 
	if(flag & 0x1)
	{
		i = NAME_MEM_SIZE;

		do
		{
			pt = (void*)realloc(filelist_infop->filename_mem, filelist_infop->mem_size+i);
			if(NULL == pt) i /= 2;
		} while(i > 256);

		if(NULL == pt) return -1;

		filelist_infop->mem_size += i;
		filelist_infop->filename_mem = (char*)pt;
		i = (int)pt - (int)filelist_infop->file_list;

		int k;
		char **m;

		k = 0;
		m = filelist_infop->file_list;
		for(k= 0; k < filelist_infop->file_num; k++)
			m[k] += i;

		k = 0;
		m = filelist_infop->dir_list;
		for(k = 0; k < filelist_infop->dir_num; k++)
			m[k] += i;
	}
	//Increase file name buffer
	if(flag & 0x2)
	{
		i = filelist_infop->file_size + FILE_LIST_MAX;

		pt = (void*)realloc(filelist_infop->file_list, i*4);
		if(NULL == pt) return -1;

		filelist_infop->file_list = (char**)pt;
		filelist_infop->file_size = i;
	}
	//Increase directory name buffer
	if(flag & 0x4)
	{
		i = filelist_infop->dir_size + DIR_LIST_MAX;

		pt = (void*)realloc(filelist_infop->dir_list, i*4);
		if(NULL == pt) return -1;

		filelist_infop->dir_list = (char**)pt;
		filelist_infop->dir_size = i;
	}

	return 0;
}

static int load_file_list(struct FILE_LIST_INFO *filelist_infop)
{
    DIR* current_dir;
    char    current_dir_name[MAX_PATH];
    dirent *current_file;
	struct stat st;
    char *file_name;
	unsigned int len;
    unsigned int file_name_length;
    char* name_mem_base;
    char **file_list;
    char **dir_list;
	unsigned int mem_size;
	unsigned int file_size;
	unsigned int dir_size;
    unsigned int num_files;
    unsigned int num_dirs;
    char **wildcards;
	char utf8[512+256];

    if(filelist_infop -> current_path == NULL)
        return -1;

    name_mem_base = &(filelist_infop -> filename_mem[0]);
    mem_size = filelist_infop -> mem_size;
	file_size = filelist_infop -> file_size;
	dir_size = filelist_infop -> dir_size;
    file_list = filelist_infop -> file_list;
    dir_list = filelist_infop -> dir_list;
    num_files = 0;
    num_dirs = 0;
    wildcards = filelist_infop -> wildcards;

    strcpy(current_dir_name, filelist_infop -> current_path);

	//*	path formate should be: "fat:/" or "fat:/dir0" or "fat:", not "fat:/dir0/"
    current_dir = opendir(current_dir_name);
	//Open directory faiure
	if(current_dir == NULL) {
		return -1;
	}

	file_name_length = 0;
	//while((current_file = readdir(current_dir)) != NULL)
	while((current_file = readdir_ex(current_dir, &st)) != NULL)
    {
		//lstat(current_file->d_name, &st);
		file_name = current_file->d_name;

		len = strlen(file_name) +1;
		if((file_name_length+len) > mem_size)
		{
			//get more memory
			if(manage_filelist_info(filelist_infop, 1) == -1)
				break;

			name_mem_base = &(filelist_infop -> filename_mem[0]);
		    mem_size = filelist_infop -> mem_size;
		}
		//Increase file_list
		if(num_files >= file_size) {
			if(manage_filelist_info(filelist_infop, 2) == -1)
				break;
			file_size = filelist_infop -> file_size;
		}
		//Increase dir_list
		if(num_dirs >= dir_size) {
			if(manage_filelist_info(filelist_infop, 4) == -1)
				break;
			dir_size = filelist_infop -> dir_size;
		}

		//If dirctory
		if(S_ISDIR(st.st_mode))
		{
			if(file_name[0] != '.')
			{
				dir_list[num_dirs] = name_mem_base + file_name_length;
				strcpy(dir_list[num_dirs], file_name);
				num_dirs++;
				file_name_length += len;
			}
			//take ".." directory as file
			else if(file_name[1] == '.')
			{
				file_list[num_files] = name_mem_base + file_name_length;
				strcpy(file_list[num_files], file_name);
				num_files++;
				file_name_length += len;
			}
		}
		else
		{
			char *ext_pos;
			unsigned int i;

			ext_pos = (char*)strrchr((const char*)file_name, '.');
			if(NULL != ext_pos)
			for(i = 0; wildcards[i] != NULL; i++)
			{
				if(!strcasecmp(ext_pos, wildcards[i]))
				{
					file_list[num_files] = name_mem_base + file_name_length;
					strcpy(file_list[num_files], file_name);
					num_files++;
					file_name_length += len;
					break;
				}
			}
		}
    }

	printf("%s:%d\n", __FILE__, __LINE__);
	printf("nfile %d; ndir %d\n", num_files, num_dirs);

    closedir(current_dir);

	filelist_infop -> file_num = num_files;
	filelist_infop -> dir_num = num_dirs;

#if 0
    my_qsort((void *)file_list, 0, num_files-1);
#else	//to support ".." directory, but take it as file
    my_qsort((void *)file_list, 1, num_files-1);
#endif
    my_qsort((void *)dir_list, 0, num_dirs-1);

    return 0;
}

extern const unsigned char font_map[128][8];

//font size 8*8
static inline void drawfont(unsigned short *addr, unsigned short f_color, unsigned short b_color, unsigned char ch)
{
	unsigned char *dot_map;
	unsigned int j, k;
	unsigned char dot;
	unsigned short *dst;

	dot_map = (unsigned char*)font_map[ch&0x7F];

	for(j= 0; j < 8; j++)
	{
		dot = *dot_map++;
		dst = addr + j*SCREEN_WIDTH;
		for(k = 0; k < 8; k++)
			*dst++ = (dot & (0x80>>k)) ? f_color : b_color;
	}
}

void drawstring(unsigned int x, unsigned int y, enum SCREEN_ID screen, char *string,
	unsigned short f_color, unsigned short b_color)
{
	unsigned short *scr_addr, *dst;

	if(screen & UP_MASK)
		scr_addr = up_screen_addr;
	else 
		scr_addr = down_screen_addr;

	if(x>= 32 || y>= 24) return;

	while(*string)
	{
		dst = scr_addr + (y*8)*SCREEN_WIDTH + x*8;
		drawfont(dst, f_color, b_color, *string++);

		x += 1;
		if(x>= 32)
		{
			x = 0;
			y+= 1;
			if(y >= 24) break;
		}
	}
}


int load_file(char **wildcards, char *result, char *default_dir_name)
{
    struct FILE_LIST_INFO filelist_info;
    int repeat;
    int return_value;
	unsigned int key;
    u32 current_selection_item;
    u32 current_in_scroll_value;
    u32 last_in_scroll_value;
    u32 last_selection_item;
    u32 directory_changed;
    char utf8[1024];
    u32 num_files;
    u32 num_dirs;
    char **file_list;
    char **dir_list;
    s32 flag;
    u32 to_update_filelist;
    u32 i, j, k;

	//initialize filelist_info struct
	if(-1 == manage_filelist_info(&filelist_info, 0))
		return -1;

    result[0] = '\0';
    if(default_dir_name != NULL)
        strcpy(filelist_info.current_path, default_dir_name);
    else
        strcpy(filelist_info.current_path, "fat:");

    filelist_info.wildcards = wildcards;
    filelist_info.file_num = 0;
    filelist_info.dir_num = 0;

    flag = load_file_list(&filelist_info);
	if(-1 == flag)
	{
		//free filelist_info struct
		manage_filelist_info(&filelist_info, -1);
		return -1;
	}

    num_files = filelist_info.file_num;
    num_dirs = filelist_info.dir_num;
    file_list = filelist_info.file_list;
    dir_list = filelist_info.dir_list;

    current_selection_item = 0;
    current_in_scroll_value = 0;
    last_in_scroll_value= -1;
    last_selection_item = -1;
    directory_changed = 1;
    to_update_filelist = 0;

    repeat= 1;
    return_value = -1;
    while(repeat)
    {
		key = getKey();
        switch(key)
        {
            case KEY_UP:
                if(current_in_scroll_value > 0)
                {
                    i= current_in_scroll_value + FILE_LIST_ROWS_CENTER;
                    if(i == current_selection_item)
                    {
                        current_in_scroll_value--;
                        current_selection_item--;
                    }
                    else if(i < current_selection_item)
                    {
                        current_selection_item--;
                    }
                    else
                    {
                        current_in_scroll_value--;
                    }
                }
                else if(current_selection_item > 0)
                    current_selection_item--;
                break;

            case KEY_DOWN:
                if(current_selection_item + 1 < num_files + num_dirs)
                {
                    current_selection_item++;
                }
                else
                {
                    if(current_in_scroll_value < current_selection_item)
                        current_in_scroll_value++;
                }

                if((current_in_scroll_value + FILE_LIST_ROWS +1) <= num_files + num_dirs)
                {
                    if((current_in_scroll_value + FILE_LIST_ROWS_CENTER) < current_selection_item)
                        current_in_scroll_value++;
                }
                break;
            //scroll page
            case KEY_RIGHT:
                if((current_in_scroll_value + FILE_LIST_ROWS) < (num_files + num_dirs))
                {
                    current_in_scroll_value += FILE_LIST_ROWS - 1;
                    current_selection_item += FILE_LIST_ROWS - 1;
                    if(current_selection_item > (num_files + num_dirs -1))
                        current_selection_item = (num_files + num_dirs -1);
                }
                break;
            //scroll page
            case KEY_LEFT:
                if(current_in_scroll_value >= (FILE_LIST_ROWS - 1))
                {
                    current_selection_item -= FILE_LIST_ROWS - 1;
                    current_in_scroll_value -= FILE_LIST_ROWS - 1;
                }
                else
                {   
                    current_selection_item -= current_in_scroll_value;
                    current_in_scroll_value = 0;
                }
                break;

            case KEY_A:
                //file selected
                if(current_selection_item + 1 <= num_files)
                {
                    if(num_files != 0)
                    {
						if(file_list[current_selection_item][0] == '.' &&
							file_list[current_selection_item][1] == '.')	//The ".." directory
						{
							char *ext_pos;

			                ext_pos = (char*)strrchr(filelist_info.current_path, '/');
							if(NULL != ext_pos)
							{
								*ext_pos = '\0';
								to_update_filelist= 1;
							}
						}
						else
						{
    	                    repeat = 0;
    	                    return_value = 0;
    	                    strcpy(result, filelist_info.current_path);
							strcat(result, "/");
							strcat(result, file_list[current_selection_item]);
						}
                    }
                }
                //dir selected
                else if(num_dirs > 0)
                {
                    u32 current_dir_selection;
					char *ext_pos;
					char ch;

                    current_dir_selection= current_selection_item -num_files;
                    if(flag >= 0)//Only last open dir not failure
                    {
						ext_pos = filelist_info.current_path;
						while(*ext_pos)
							ch = *ext_pos++;

						if('/' != ch) *ext_pos++ = '/';

						strcpy(ext_pos, dir_list[current_dir_selection]);
                        to_update_filelist= 1;
                    }
                }

                break;

            case KEY_B:
				{
					char *ext_pos;

    	            ext_pos = (char*)strrchr(filelist_info.current_path, '/');
					if(NULL != ext_pos) {
						*ext_pos = '\0';
						to_update_filelist= 1;
					}
				}
                break;

            default:
                break;
        }//end switch

        if(to_update_filelist)
        {
			char *ext_pos;

            flag = load_file_list(&filelist_info);
			if(-1 == flag) {
				ext_pos = (char*)strrchr(filelist_info.current_path, '/');
				*ext_pos = '\0';
			}

            num_files = filelist_info.file_num;
            num_dirs = filelist_info.dir_num;

            current_selection_item = 0;
            current_in_scroll_value = 0;
            last_in_scroll_value= -1;
            last_selection_item = -1;
            directory_changed = 1;

            to_update_filelist = 0;
        }

        if(current_in_scroll_value != last_in_scroll_value || current_selection_item != last_selection_item)
        {
			ds2_clearScreen(DOWN_SCREEN, 0x7FFF);

            //file path
            if(directory_changed)
            {
				drawstring(0, 0, DOWN_SCREEN, filelist_info.current_path, BLACK_COLOR, WHITE_COLOR);
            }

            j= 0;
            k= current_in_scroll_value;
            i = num_files + num_dirs - k;
            if(i > FILE_LIST_ROWS) i= FILE_LIST_ROWS;

            for( ; i> 0; i--)
            {
                u16 color;

                if(k == current_selection_item)
                    color= GREEN_COLOR;
                else
                    color= BLACK_COLOR;

                if(k < num_files)
                {
					int len;

					strcpy(utf8, file_list[k]);
					len = strlen(utf8);

					if(len >= 28) utf8[28] = '\0';
					drawstring(0, j+4, DOWN_SCREEN, utf8, color, WHITE_COLOR);
                }
                else
                {
					int len;

					strcpy(utf8, dir_list[k-num_files]);
					len = strlen(utf8);

					if(len >= 28) utf8[28] = '\0';
					drawstring(0, j+4, DOWN_SCREEN, utf8, color, WHITE_COLOR);

					strcpy(utf8, "D");
					drawstring(31, j+4, DOWN_SCREEN, utf8, color, WHITE_COLOR);
                }

                k++, j++;
            }

            last_in_scroll_value = current_in_scroll_value;
            last_selection_item = current_selection_item;

			ds2_flipScreen(DOWN_SCREEN, 1);
        }

        mdelay(50);		//about 100ms
    }

	//free filelist_info struct
	manage_filelist_info(&filelist_info, -1);

    ds2_clearScreen(DOWN_SCREEN, BLACK_COLOR);
    ds2_flipScreen(DOWN_SCREEN, 1);

    return return_value;
}

#include "bitmap.h"

int show_bmp(char *file)
{
	unsigned int bmp_type;
	unsigned char *bmp_buf;
	unsigned short *dst;	
	unsigned int x, y;
	unsigned int key;
	unsigned int width, height, w, h;
	unsigned int flag;

	bmp_buf = (char*)malloc(SCREEN_WIDTH*SCREEN_HEIGHT*4);
	if(NULL == bmp_buf)
		return -1;

	memset(bmp_buf, 0, SCREEN_WIDTH*SCREEN_HEIGHT*4);
	ds2_clearScreen(DOWN_SCREEN, 0);

	width = SCREEN_WIDTH;
	height = SCREEN_HEIGHT;
	if(BMP_read(file, bmp_buf, &width, &height, &bmp_type) !=BMP_OK )
	{
		free((void*)bmp_buf);
		return -1;
	}

	w = (SCREEN_WIDTH - width)/2;
	h = (SCREEN_HEIGHT - height)/2;

	printf("w %d; h %d\n", width, height);

	if(bmp_type ==2)		//2 bytes per pixel
	{
		unsigned short *pt;

		for(y= 0; y< height; y++)
		{
			dst = (unsigned short*)down_screen_addr + (y+h)*SCREEN_WIDTH +w;
			pt = (unsigned short*)bmp_buf + y*SCREEN_WIDTH*2;
			for(x= 0; x< width; x++)
			{
				*dst++= RGB16_15(pt);
				pt += 1;
			}
		}
	}
	else if(bmp_type ==3)	//3 bytes per pixel
	{
		unsigned char *buff;

		for(y= 0; y< height; y++)
		{
			dst = (unsigned short*)down_screen_addr + (y+h)*SCREEN_WIDTH +w;
			buff = bmp_buf + y*SCREEN_WIDTH*4;
			for(x= 0; x< width; x++)
			{
				*dst++= RGB24_15(buff);
				buff += 3;
			}
		}
	}
	else
	{
		free((void*)bmp_buf);
		return -1;
	}

	ds2_flipScreen(DOWN_SCREEN, 1);
	free((void*)bmp_buf);

	int backlight = 3;
	int brightness = ds2_getBrightness();

	flag = 1;
	while(flag)
	{
		key = getKey();
		switch(key)
		{
			case KEY_UP:
					backlight ^= 2;
					ds2_setBacklight(backlight);
				break;

			case KEY_DOWN:
					backlight ^= 1;
					ds2_setBacklight(backlight);
				break;

			case KEY_LEFT:
					brightness += 1;
					if(brightness > 3) brightness = 3;

					ds2_setBrightness( brightness );
				break;

			case KEY_RIGHT:
					brightness -= 1;
					if(brightness < 0) brightness = 0;

					ds2_setBrightness( brightness );
				break;

			case KEY_L:
                    ds2_plug_exit();
                    //ds2_setSupend();
				break;

			case KEY_R:
                    ds2_shutdown();
	                //ds2_wakeup();
				break;

			case KEY_B:
					flag = 0;
				break;
		}
	}

	return 0;
}

void playwave(char *file)
{
	if(play_wave(file) == -1)
		printf("Play: %s failure\n", file);
}

//NOTE: don't using printf in interrupt handle, because the global interrupt is
//	disabled, the ds2io hardware layer is suspend without interrupt, the video and
// audio data can't send to NDS, key information can't received also, so printf 
// will suspend the DSTWO.
// You can use sti() force global interrupt enable, but printf still seems to 
// cause some problem...
// for example, the timer interrupt period set to 10ms, but the printf need 10ms
// or more to complete, the interrupt will nested forever till the stack run over
void timerhandle(unsigned int arg)
{
//	sti();
//	printf("T %d\n", GetSysTime());
}


////////////////////////////////////////////////////////////////////////////////
void main(int argc, char* argv[])
{
	char *wildcards[]= {".bmp", ".wav", NULL};
	char result[512];
	char filepath[512];
	char *pt;

	printf("Hello ds2sdk\r\n");
	printf("%s\n", ds2_getVersion());

	ds2_setCPUclocklevel(0);
	printf_clock();


#if 0
	while(1)
	{
    	unsigned int key;
	    struct key_buf input;
    
		key = getInput(&input);

	    if(key)
	    {
	        if(input.key & KEY_TOUCH)
	            printf("x,y = (%d, %d)\n", input.x, input.y);
	        else if(input.key & KEY_LID)
	            printf("KEY_LID\n");
	        else if(input.key & KEY_UP)
	            printf("KEY_UP\n");
	        else if(input.key & KEY_DOWN)
	            printf("KEY_DOWN\n");
	        else if(input.key & KEY_LEFT)
	            printf("KEY_LEFT\n");
	        else if(input.key & KEY_RIGHT)
	            printf("KEY_RIGHT\n");
	        else if(input.key & KEY_L)
	            printf("KEY_L\n");
	        else if(input.key & KEY_R)
	            printf("KEY_R\n");
	        else if(input.key & KEY_A)
	            printf("KEY_A\n");
	        else if(input.key & KEY_B)
	            printf("KEY_B\n");
	        else if(input.key & KEY_X)
	            printf("KEY_X\n");
	        else if(input.key & KEY_Y)
	            printf("KEY_Y\n");
	        else if(input.key & KEY_START)
	            printf("KEY_START\n");
	        else if(input.key & KEY_SELECT)
	            printf("KEY_SELECT\n");
	    }
	}
#endif

	//Interrupt period =1s
	initTimer(1, 1000000, timerhandle, 0);
	runTimer(1);

	strcpy(filepath, "fat:");
	while(1)
	{
		if(load_file(wildcards, result, filepath) == -1)
			break;
		pt = (char*)strrchr(result, '.');
		if(!strcasecmp(pt, wildcards[0]))
		{
			show_bmp(result);
		}
		else
		{
			playwave(result);
		}

		pt = (char*)strrchr(result, '/');
		if(NULL != pt)
			*pt = '\0';

		strcpy(filepath, result);
	}

	printf("load file list failure\n");
	while(1);
}


